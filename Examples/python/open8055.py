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
# Constants
# ----------
MODE_ADC10      = 10
MODE_ADC9       = 11
MODE_ADC8       = 12

MODE_INPUT      = 20
MODE_FREQUENCY  = 21
MODE_EUSART     = 22

MODE_OUTPUT     = 30
MODE_SERVO      = 31
MODE_ISERVO     = 32

MODE_PWM        = 40


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
        self.autoflush      = False     # If to send changes immediately
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
                hid_type = args[1][0:2]
                if hid_type == '01':
                    self.last_output = args[1]
                elif hid_type == '03':
                    self.last_config1 = args[1]
                elif hid_type == '81':
                    self.last_input = args[1]
                else:
                    raise Exception('unexpected HID report type 0x' + hid_type +
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
        # Set the status of the receiver thread to stop, so that
        # it will exit on the next HID report received.
        # Then send a GETINPUT message, forcing the card to produce
        # an INPUT even if no input has changed.
        # ----
        self.receiver.set_status('STOP')
        self.sock.sendall('SEND 02\n')

        # ----
        # Wait for the receiver thread to terminate.
        # ----
        self.receiver.join()

        # ----
        # Shutdown and close the server connection.
        # ----
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

        # ----
        # Finally check that the receiver didn't detect an error.
        # ----
        if self.receiver.status != 'STOPPED':
            raise Exception(self.receiver.error)


    def get_input_all(self):
        self.lock.acquire()
        result = int(self.last_input[2:4], 16) & 0xFF
        self.lock.release()
        return result

    def get_input_port(self, port):
        if port < 0 or port > 4:
            raise ValueError('illegal digital input port number')
        self.lock.acquire()
        result = (int(self.last_input[2:4], 16) & (1 << port)) != 0
        self.lock.release()
        return result

    def get_counter(self, port):
        if port < 0 or port > 4:
            raise ValueError('illegal digital input port number')
        index = port * 4 + 4
        self.lock.acquire()
        result = int(self.last_input[index:index+4], 16) & 0xFFFF
        self.lock.release()
        return result

    def get_adc(self, port):
        if port < 0 or port > 1:
            raise ValueError('illegal adc input port number')
        index = port * 4 + 24
        self.lock.acquire()
        result = int(self.last_input[index:index+4], 16) & 0xFFFF
        self.lock.release()
        return result

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
                hid_type = args[1][0:2]
                conn.lock.acquire()
                if hid_type == '81':
                    conn.last_input = args[1]
                elif hid_type == '01':
                    conn.last_output = args[1]
                elif hid_type == '03':
                    conn.last_config1 = args[1]
                else:
                    conn.lock.release()
                    self.lock.acquire()
                    self.error = 'unknown HID message type 0x' + hid_type
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

