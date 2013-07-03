# ----------------------------------------------------------------------
# open8055.py
#
#   A python class to communicate with the open8055server.
#
# ----------------------------------------------------------------------
#
#  Copyright (c) 2013, Jan Wieck
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#      * Neither the name of the <organization> nor the
#        names of its contributors may be used to endorse or promote products
#        derived from this software without specific prior written permission.
#  
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#  
# ----------------------------------------------------------------------

import sys, os, socket, threading, Queue

# ----------
# Port configuration mode values
# ----------
MODE_ADC10      = 10
MODE_ADC9       = 11
MODE_ADC8       = 12

MODE_INPUT      = 20
MODE_FREQUENCY  = 21

MODE_OUTPUT     = 30
MODE_SERVO      = 31
MODE_ISERVO     = 32

MODE_PWM        = 40

# ----------
# HID command and report types encoded in the first byte
# ----------
OUTPUT          = 0x01
GETINPUT        = 0x02
SETCONFIG1      = 0x03
GETCONFIG       = 0x04
SAVECONFIG      = 0x05
SAVEALL         = 0x06
RESET           = 0x7F
INPUT           = 0x81


# ----------
# The open8055 class
# ----------
class open8055:
    def __init__(self, cardid, 
            host='localhost', port=8055, user='nobody', password=None,
            timeout=None):
        # ----
        # Open the connection and initialize object variables
        # ----
        self.sock, self.server_version, salt = _connect(
                host, port, user, password, timeout)
        self.last_input     = None      # Last HID report of type INPUT
        self.last_config1   = None      # Current CONFIG1 data
        self.last_output    = None      # Current OUTPUT data
        self.inbuf          = ''        # Raw input buffer
        self.listener       = None      # Somebody is waiting for input
        self.autoflush      = True      # If to send changes immediately
        self.pend_output    = False     # Pending OUTPUT to flush
        self.pend_config1   = False     # Pending CONFIG1 to flush
        self.lock           = threading.Lock()

        # ----
        # Send the OPEN command
        # ----
        md5_password = _encrypt_password(user, password, salt)
        self.sock.sendall('OPEN ' + str(cardid) + ' ' + 
                user + ' ' + md5_password + '\n')

        # ----
        # The Open8055Server will make sure we get CONFIG1, OUTPUT
        # and a forced INPUT report after connecting.
        # Process the data until we have all of that.
        # ----
        while not self.last_config1 or not self.last_output or \
                not self.last_input:
            args, self.inbuf = _recv(self.sock, self.inbuf)
            if args[0] == 'RECV':
                hid_type = int(args[1])
                values = [int(x) for x in args[1:]]
                if hid_type == INPUT:
                    self.last_input = dict((
                            ('msgType',         hid_type),
                            ('inputBits',       values[1]),
                            ('inputCounter',    values[2:7]),
                            ('inputADCValue',   values[7:9]),
                        ))
                elif hid_type == OUTPUT:
                    self.last_output = dict((
                            ('msgType',         hid_type),
                            ('outputBits',      values[1]),
                            ('outputValue',     values[2:10]),
                            ('outputPWMValue',  values[10:12]),
                            ('resetCounter',    values[12]),
                        ))
                elif hid_type == SETCONFIG1:
                    self.last_config1 = dict((
                            ('msgType',         hid_type),
                            ('modeADC',         values[1:3]),
                            ('modeInput',       values[3:8]),
                            ('modeOutput',      values[8:16]),
                            ('modePWM',         values[16:18]),
                            ('debounceValue',   values[18:23]),
                            ('cardAddress',     values[23]),
                        ))
                else:
                    raise Exception('unexpected HID report type ' +
                            '0x{0:02X}'.format(hid_type) +
                            ' in startup mode')
            elif args[0] == 'ERROR':
                try:
                    self.sock.shutdown(socket.SHUT_RDRW)
                    self.sock.close()
                finally:
                    raise Exception(' '.join(args[1:]))
            else:
                try:
                    self.sock.shutdown(socket.SHUT_RDRW)
                    self.sock.close()
                finally:
                    raise Exception('unexpected server reply: "' + 
                            ' '.join(args) + '"')
        # ----
        # We are fully connected and have the current status of the card.
        # Switch off timeouts and start the receiver thread.
        # ----
        self.sock.settimeout(None)
        self.sock.setblocking(True)
        self.receiver = _open8055receiver(self)
        self.receiver.start()


    def close(self):
        # ----
        # Try to flush any pending output to the client, just in case.
        # ----
        try:
            self.flush()
        except:
            pass

        # ----
        # Set the status of the receiver thread to stop, so that
        # it will exit on the next HID report received.
        # Then send a GETINPUT message, forcing the card to produce
        # an INPUT even if no input has changed.
        # ----
        self.receiver.set_status('STOP')
        self.sock.sendall('SEND {0}\n'.format(GETINPUT))

        # ----
        # Wait for the receiver thread to terminate.
        # ----
        self.receiver.join()

        # ----
        # Send QUIT to the server and wait for it to close connection.
        # ----
        error = False
        self.sock.sendall('QUIT\n')
        while True:
            args, self.inbuf = _recv(self.sock, self.inbuf)
            if not args:
                break
            if args[0] == 'ERROR':
                error = ' '.join(args[1:])

        # ----
        # Shutdown and close the server connection.
        # ----
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

        # ----
        # Finally check if there were any errors along the way.
        # ----
        if self.receiver.status != 'STOPPED':
            raise Exception(self.receiver.error)
        if error:
            raise Exception(error)


    def get_input_all(self):
        self.lock.acquire()
        result = self.last_input['inputBits']
        self.lock.release()
        return result

    def get_input_port(self, port):
        if port < 0 or port > 4:
            raise ValueError('illegal digital input port number')
        self.lock.acquire()
        result = result = (self.last_input['inputBits'] & (1 << port)) != 0
        self.lock.release()
        return result

    def get_counter(self, port):
        if port < 0 or port > 4:
            raise ValueError('illegal digital input port number')
        self.lock.acquire()
        result = self.last_input['inputCounter'][port]
        self.lock.release()
        return result

    def get_adc(self, port):
        if port < 0 or port > 1:
            raise ValueError('illegal adc input port number')
        self.lock.acquire()
        result = self.last_input['inputADCValue'][port]
        self.lock.release()
        return result

    def set_output_all(self, value):
        value &= 0xFF
        self.lock.acquire()
        self.last_output['outputBits'] = value
        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True
        self.lock.release()

    def get_output_all(self):
        self.lock.acquire()
        result = self.last_output['outputBits']
        self.lock.release()
        return result

    def get_output_port(self, port):
        if port < 0 or port > 7:
            raise ValueError('illegal digital output port number')
        self.lock.acquire()
        result = (self.last_output['outputBits'] & (1 << port)) != 0
        self.lock.release()
        return result

    def set_output_port(self, port, value):
        if port < 0 or port > 7:
            raise ValueError('illegal digital output port number')
        self.lock.acquire()
        if value:
            self.last_output['outputBits'] |= (1 << port)
        else:
            self.last_output['outputBits'] &= ~(1 << port)
        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True
        self.lock.release()

    def get_output_value(self, port):
        if port < 0 or port > 7:
            raise ValueError('illegal digital output port number')
        self.lock.acquire()
        result = self.last_output['outputValue'][port]
        self.lock.release()
        return result

    def set_output_value(self, port, value):
        if port < 0 or port > 7:
            raise ValueError('illegal digital output port number')
        if value < 0 or value > 65535:
            raise ValueError('illegal digital output value')
        self.lock.acquire()
        self.last_output['outputValue'][port] = int(value)
        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True
        self.lock.release()

    def get_output_servo(self, port):
        millisec = self.get_output_value(port) / 12000.0
        if millisec < 0.5:
            millisec = 0.5
        if millisec > 2.5:
            millisec = 2.5
        return millisec

    def set_output_servo(self, port, millisec):
        if port < 0 or port > 7:
            raise ValueError('illegal digital output port number')
        if millisec < 0.5 or millisec > 2.5:
            raise ValueError('illegal value for servo pulse width')
        self.set_output_value(port, int(millisec * 12000.0)

    def get_pwm(self, port):
        if port < 0 or port > 1:
            raise ValueError('illegal pwm port number')
        self.lock.acquire()
        result = self.last_output['outputPWMValue'][port]
        self.lock.release()
        return result

    def set_pwm(self, port, value):
        if port < 0 or port > 1:
            raise ValueError('illegal pwm port number')
        if value < 0 or value > 1023:
            raise ValueError('illegal pwm value')
        self.lock.acquire()
        self.last_output['outputPWMValue'][port] = int(value)
        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True
        self.lock.release()

    def get_autoflush(self):
        self.lock.acquire()
        result = self.autoflush
        self.lock.release()
        return result

    def set_autoflush(self, state):
        self.lock.acquire()
        self.autoflush = (state != False)
        if state:
            if self.pend_output:
                self._send_output()
                self.pend_output = False
        self.lock.release()

    def flush(self):
        self.lock.acquire()
        if self.pend_output:
            self._send_output()
            self.pend_output = False
        self.lock.release()

    def _send_output(self):
        msg = ' '.join((
                'SEND',
                str(self.last_output['msgType']),
                str(self.last_output['outputBits']),
                ' '.join(
                    [str(y) for y in self.last_output['outputValue']]),
                ' '.join(
                    [str(y) for y in self.last_output['outputPWMValue']]),
                str(self.last_output['resetCounter'])
            )) + '\n'
        self.sock.sendall(msg)

class _open8055receiver(threading.Thread):
    def __init__(self, conn):
        threading.Thread.__init__(self)

        self.conn       = conn
        self.status     = 'RUN'
        self.lock       = threading.Lock()

    def run(self):
        conn = self.conn
        while self.get_status() == 'RUN':
            try:
                args, conn.inbuf = _recv(conn.sock, conn.inbuf)
            except Exception as err:
                self.error = str(err)
                self.status = 'ERROR'
                return

            if args[0] == 'RECV':
                hid_type = int(args[1])
                values = [int(x) for x in args[1:]]
                conn.lock.acquire()
                if hid_type == INPUT:
                    conn.last_input = dict((
                            ('msgType',         hid_type),
                            ('inputBits',       values[1]),
                            ('inputCounter',    values[2:7]),
                            ('inputADCValue',   values[7:9]),
                        ))
                elif hid_type == OUTPUT:
                    conn.last_output = dict((
                            ('msgType',         hid_type),
                            ('outputBits',      values[1]),
                            ('outputValue',     values[2:10]),
                            ('outputPWMValue',  values[10:12]),
                            ('resetCounter',    values[12]),
                        ))
                elif hid_type == SETCONFIG1:
                    conn.last_config1 = dict((
                            ('msgType',         hid_type),
                            ('modeADC',         values[1:3]),
                            ('modeInput',       values[3:8]),
                            ('modeOutput',      values[8:16]),
                            ('modePWM',         values[16:18]),
                            ('debounceValue',   values[18:23]),
                            ('cardAddress',     values[23]),
                        ))
                else:
                    conn.lock.release()
                    self.lock.acquire()
                    self.error = 'unknown HID message type 0x{0:02X}'.format(
                            hid_type);
                    self.status = 'ERROR'
                    self.lock.release()
                    return
                
                if conn.listener:
                    conn.listener.put(args)

                conn.lock.release()

            elif args[0] == 'ERROR':
                self.lock.acquire()
                self.error = ' '.join(args[1:])
                self.status = 'ERROR'
                self.lock.release()
                conn.lock.acquire()
                if conn.listener:
                    conn.listener.put(args)
                conn.lock.release()
                return

            else:
                msg = 'unexpected server reply: "' + \
                        ' '.join(args) + '"'
                self.lock.acquire()
                self.error = msg
                self.status = 'ERROR'
                self.lock.release()
                conn.lock.acquire()
                if conn.listener:
                    conn.listener.put(('ERROR', msg))
                conn.lock.release()
                return
        
        self.lock.acquire()
        if self.status != 'ERROR':
            self.status = 'STOPPED'
        self.lock.release()

        conn.lock.acquire()
        if conn.listener:
            conn.listener.put(('STOPPED', ))
        conn.lock.release()


    def get_status(self):
        self.lock.acquire()
        result = self.status
        self.lock.release()
        return result
        
    def set_status(self, new):
        self.lock.acquire()
        self.status = new
        self.lock.release()
        

# ----------
# cards()
#
#   Utility function to get the list of available cardids.
# ----------
def cards(host='localhost', port=8055, user='nobody', password='nopass',
        timeout=None):
    # ----
    # Open the connection
    # ----
    sock, server_version, salt = _connect(host, port, user, password, timeout)
    inbuf = ''

    # ----
    # Send the LIST command
    # ----
    md5_password = _encrypt_password(user, password, salt)
    sock.sendall('LIST ' + user + ' ' + md5_password + '\n')

    # ----
    # Wait for the LIST reply
    # ----
    while True:
        args, inbuf = _recv(sock, inbuf)
        if args[0].upper() == 'LIST':
            break
        elif args[0].upper() == 'ERROR':
            try:
                sock.shutdown(socket.SHUT_RDRW)
                sock.close()
            finally:
                raise Exception(' '.join(args[1:]))
        else:
            try:
                sock.shutdown(socket.SHUT_RDRW)
                sock.close()
            finally:
                raise Exception('unexpected server reply: "', + 
                        ' '.join(args) + '"')

    # ----
    # Close the connection.
    # ----
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

    # ----
    # Return the result
    # ----
    result = []
    for card in args[1:]:
        result.append(int(card))
    return result


def _encrypt_password(user, password, salt):
    ##########
    # TODO: actually encrypt the thing or look it up in ~/.open8055passwd
    ##########
    if not password:
        return 'nopass'
    return password


def _connect(host='localhost', port=8055, user='nobody', password='nopass',
        timeout=None):
    # ----
    # Open the host connection and set the timeout.
    # ----
    sock = socket.create_connection((host, port))
    sock.settimeout(timeout)
    inbuf = ''

    # ----
    # Get the HELLO message and remember the server version
    # ----
    args, inbuf = _recv(sock, inbuf)
    if args[0] != 'HELLO' or args[1] != 'Open8055Server':
        try:
            sock.shutdown(socket.SHUT_RDWR)
            sock.close()
        finally:
            raise Exception('Expected "HELLO Open8055Server <version>" - ' +
                    'got "' + line + '" instead')
    version = args[2]

    # ----
    # Get the SALT
    # ----
    args, inbuf = _recv(sock, inbuf)
    if args[0].upper() != 'SALT':
        try:
            sock.shutdown(socket.SHUT_RDWR)
            sock.close()
        finally:
            raise Exception('Expected "SALT <hexdata>" - ' +
                    'got "' + line + '" instead')
    salt = args[1]

    return (sock, version, salt)


def _recv(sock, buf, poll=False):
    idx = buf.find('\n')
    while idx < 0:
        data = sock.recv(4096)
        if not data:
            return None, buf
        buf += data
        idx = buf.find('\n')
        if idx < 0 and poll:
            return '', inbuf

    return buf[0:idx].split(' '), buf[idx + 1:]

