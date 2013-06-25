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

import sys, os, socket, threading
import Queue

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


class Open8055:
    def __init__(self):
        self.sock           = None
        self.fdin           = None
        self.lock           = threading.Lock()
        self.cardfifo       = {}
        self.autosend_flag  = {}
        self.output_data    = {}
        self.input_data     = {}
        self.config1_data   = {}

    # ----------
    # connect()
    #
    #   Connect to the server and authenticate.
    # ----------
    def connect(self, host='localhost', port=8055, timeout=None,
            user='nobody', password=''):

        # ----
        # Make sure we aren't already connected.
        # ----
        if self.sock:
            raise Exception('already connected')

        # ----
        # Connect to the open8055server and create a file for reading.
        # ----
        sock = socket.create_connection([host, port], timeout)
        fdin = os.fdopen(sock.fileno(), 'r')

        # ----
        # Get the HELLO and SALT messages from the server.
        # ----
        self.hello  = fdin.readline().strip().split(' ');
        if self.hello[0] != 'HELLO' or self.hello[1] != 'Open8055server':
            fdin.close()
            raise Exception('Server did not send HELLO message');
        salt = fdin.readline().strip().split(' ')
        if salt[0] != 'SALT':
            fdin.close()
            raise Exception('Server did not send SALT message');

        # ----
        # Authenticate with LOGIN.
        # ----
        sock.sendall('LOGIN ' + user + ' ' + password + '\n')
        line = fdin.readline().strip()
        reply = line.split(' ')
        if reply[0] != 'LOGIN':
            fdin.close()
            raise Exception('expected LOGIN reply, got \'' + line + '\'')
        if reply[1] != 'OK':
            fdin.close()
            raise Exception('Login failed')
        
        self.sock = sock
        self.fdin = fdin
        self.reader = Open8055._reader(self)
        self.reader.daemon = True
        self.reader.start()
        self.listfifo = Queue.Queue()


    # ----------
    # disconnect()
    #
    #   Close communication and wait for the reader thread.
    # ----------
    def disconnect(self):
        self.lock.acquire()
        if self.fdin:
            self.sock.sendall('BYE\n');
            self.lock.release()
            self.reader.join()
            self.lock.acquire()

            self.fdin.close()
            self.fdin = None
            self.sock = None
        self.lock.release()


    # ----------
    # list()
    #
    #   Returns a list of available cards.
    # ----------
    def list(self):
        self.lock.acquire()
        self.sock.sendall('LIST\n')
        fifo = self.listfifo
        self.lock.release()
        reply = fifo.get()
        if reply[0] == 'OK':
            result = []
            for cardid in reply[1:]:
                result.append(int(cardid))
            return result
        raise Exception(' '.join(result[1:]))


    # ----------
    # open()
    #
    #   Open a card. This also sends the GETCONFIG request and
    #   waits for the current configuration and state be reported.
    # ----------
    def open(self, cardid):
        self.lock.acquire()
        sock = self.sock
        self.lock.release()

        # ----
        # First we send the OPEN command and expect 'OPEN cardid OK'
        # ----
        fifo = self.listen(cardid)
        sock.sendall('OPEN ' + str(cardid) + '\n')
        result = fifo.get()
        if result != ['OPEN', str(cardid), 'OK']:
            self.unlisten(cardid)
            raise Exception(' '.join(result[3:]))

        # ----
        # Now send the GETCONFIG command and wait for all messages
        # we expect in return.
        # ----
        sock.sendall('SEND ' + str(cardid) + ' 04\n')
        have_input      = False
        have_output     = False
        have_config1    = False

        while not have_input or not have_output or not have_config1:
            result = fifo.get()

            if result[0:3] == ['RECV', str(cardid), 'DATA']:
                data = result[3]
                code = data[0:2]

                if code == '01':
                    self.output_data[cardid] = data
                    have_output = True

                elif code == '03':
                    self.config1_data[cardid] = data
                    have_config1 = True

                elif code == '81':
                    if have_output and have_config1:
                        self.input_data[cardid] = data
                        have_input = True

            else:
                self.unlisten(cardid)
                raise Exception(' '.join(result))

        # ----
        # Stop listening for messages on the card fifo. User
        # can turn this on/off as needed later.
        # ----
        self.unlisten(cardid)

        # ----
        # Set the card to autosend mode.
        # ----
        self.lock.acquire()
        self.autosend_flag[cardid] = True
        self.lock.release()


    # ----------
    # autosend()
    #
    #   Get or set the autosend feature for a card. In autosend mode,
    #   any change to a single port automatically causes send().
    # ----------
    def autosend(self, cardid, mode=None):
        self.lock.acquire()
        ret = self.autosend_flag[cardid]
        if mode != None:
            self.autosend_flag[cardid] = mode
        self.lock.release()
        return ret

    # ----------
    # send()
    #
    #   Send the current OUTPUT data to the card.
    # ----------
    def send(self, cardid):
        self.lock.acquire()
        self.sock.sendall('SEND ' + str(cardid) + ' ' + \
            self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # send_config()
    #
    #   Send the current CONFIG1 data to the card.
    # ----------
    def send(self, cardid):
        self.lock.acquire()
        self.sock.sendall('SEND ' + str(cardid) + ' ' + \
            self.config1_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # listen()
    #
    #   Create a queue to receive HID packets when they arive from a card.
    # ----------
    def listen(self, cardid):
        self.lock.acquire()
        if cardid in self.cardfifo:
            self.lock.release()
            raise Exception('card ' + str(cardid) + \
                    'already has a listening queue')
        fifo = Queue.Queue()
        self.cardfifo[cardid] = fifo
        self.lock.release()
        return fifo

    # ----------
    # unlisten()
    #
    #   Stop listening on a card.
    # ----------
    def unlisten(self, cardid):
        self.lock.acquire()
        if not cardid in self.cardfifo:
            self.lock.release()
            raise Exception('card ' + str(cardid) + \
                    'does not have a listening queue')
        del self.cardfifo[cardid]
        self.lock.release()

    # ----------
    # ----------
    def get_input_all(self, cardid):
        self.lock.acquire()
        ret = int(self.input_data[cardid][2:4], 16) & 0xFF
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_input_port(self, cardid, port):
        if port < 0 or port > 4:
            return 0
        if self.get_input_all(cardid) & (1 << port):
            return 1
        return 0

    # ----------
    # ----------
    def get_input_counter(self, cardid, port):
        if port < 0 or port > 4:
            return 0
        self.lock.acquire()
        idx = 4 + port * 4
        ret = int(self.input_data[cardid][idx:idx + 4], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_input_adc(self, cardid, port):
        if port < 0 or port > 1:
            return 0
        self.lock.acquire()
        idx = 24 + port * 4
        ret = int(self.input_data[cardid][idx:idx + 4], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_output_all(self, cardid):
        self.lock.acquire()
        ret = int(self.output_data[cardid][2:4], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_output_port(self, cardid, port):
        if port < 0 or port > 7:
            return 0
        if self.get_output_all(cardid) & (1 << port):
            return 1
        return 0
        
    # ----------
    # ----------
    def get_output_value(self, cardid, port):
        if port < 0 or port > 7:
            return 0
        self.lock.acquire()
        idx = 4 + port * 4
        ret = int(self.output_data[cardid][idx:idx + 4], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_output_pwm(self, cardid, port):
        if port < 0 or port > 1:
            return 0
        self.lock.acquire()
        idx = 36 + port * 4
        ret = int(self.output_data[cardid][idx:idx + 4], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def set_output_all(self, cardid, val):
        self.lock.acquire()
        self.output_data[cardid] = '01{0:02X}'.format(val & 0xFF) + \
                self.output_data[cardid][4:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_output_port(self, cardid, port, val):
        if port < 0 or port > 7:
            return

        self.lock.acquire()
        out = int(self.output_data[cardid][2:4], 16) & 0xFF
        if val:
            out |= (1 << port)
        else:
            out &= ~(1 << port)

        self.output_data[cardid] = '01{0:02X}'.format(out & 0xFF) + \
                self.output_data[cardid][4:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_output_value(self, cardid, port, val):
        if port < 0 or port > 7:
            return

        self.lock.acquire()
        idx = 4 + port * 4
        self.output_data[cardid] = self.output_data[cardid][0:idx] + \
                '{0:04X}'.format(val & 0xFFFF) + \
                self.output_data[cardid][idx + 4:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_output_pwm(self, cardid, port, val):
        if port < 0 or port > 1:
            return

        self.lock.acquire()
        idx = 36 + port * 4
        self.output_data[cardid] = self.output_data[cardid][0:idx] + \
                '{0:04X}'.format(val & 0x03FF) + \
                self.output_data[cardid][idx + 4:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def reset_counter(self, cardid, port):
        if port < 0 or port > 4:
            return

        self.lock.acquire()
        idx = 44
        val = int(self.output_data[cardid][idx:idx + 2], 16)
        val |= (1 << port)
        self.output_data[cardid] = self.output_data[cardid][0:idx] + \
                '{0:02X}'.format(val) + \
                self.output_data[cardid][idx + 2:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.output_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def get_config_adc(self, cardid, port):
        if port < 0 or port > 1:
            return 0
        self.lock.acquire()
        idx = 2 + port * 2
        ret = int(self.config1_data[cardid][idx:idx + 2], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_config_input(self, cardid, port):
        if port < 0 or port > 4:
            return 0
        self.lock.acquire()
        idx = 6 + port * 2
        ret = int(self.config1_data[cardid][idx:idx + 2], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_config_output(self, cardid, port):
        if port < 0 or port > 7:
            return 0
        self.lock.acquire()
        idx = 16 + port * 2
        ret = int(self.config1_data[cardid][idx:idx + 2], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def get_config_pwm(self, cardid, port):
        if port < 0 or port > 1:
            return 0
        self.lock.acquire()
        idx = 32 + port * 2
        ret = int(self.config1_data[cardid][idx:idx + 2], 16)
        self.lock.release()
        return ret

    # ----------
    # ----------
    def set_config_adc(self, cardid, port, mode):
        if port < 0 or port > 1:
            return
        if mode < MODE_ADC10 or mode > MODE_ADC8:
            return

        self.lock.acquire()
        idx = 2 + port * 2
        self.config1_data[cardid] = self.config1_data[cardid][0:idx] + \
                '{0:02X}'.format(mode) + \
                self.config1_data[cardid][idx + 2:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.config1_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_config_input(self, cardid, port, mode):
        if port < 0 or port > 4:
            return
        if mode < MODE_INPUT or mode > MODE_FREQUENCY:
            return

        self.lock.acquire()
        idx = 6 + port * 2
        self.config1_data[cardid] = self.config1_data[cardid][0:idx] + \
                '{0:02X}'.format(mode) + \
                self.config1_data[cardid][idx + 2:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.config1_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_config_output(self, cardid, port, mode):
        if port < 0 or port > 7:
            return
        if mode < MODE_OUTPUT or mode > MODE_ISERVO:
            return

        self.lock.acquire()
        idx = 16 + port * 2
        self.config1_data[cardid] = self.config1_data[cardid][0:idx] + \
                '{0:02X}'.format(mode) + \
                self.config1_data[cardid][idx + 2:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.config1_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # ----------
    def set_config_pwm(self, cardid, port, mode):
        if port < 0 or port > 1:
            return
        if mode != MODE_PWM:
            return

        self.lock.acquire()
        idx = 32 + port * 2
        self.config1_data[cardid] = self.config1_data[cardid][0:idx] + \
                '{0:02X}'.format(mode) + \
                self.config1_data[cardid][idx + 2:]
        if self.autosend_flag[cardid]:
            self.sock.sendall('SEND ' + str(cardid) + ' ' + \
                self.config1_data[cardid] + '\n')
        self.lock.release()

    # ----------
    # _reader()
    # ----------
    class _reader(threading.Thread):
        def __init__(self, conn):
            threading.Thread.__init__(self)
            self.conn   = conn

        def run(self):
            while True:
                try:
                    line = self.conn.fdin.readline().strip()
                    if not line:
                        break
                
                except Exception as err:
                    print '_reader(): error:', err
                    break

                tags = line.split(' ')
                if tags[0] == 'LIST':
                    self.conn.listfifo.put(tags[1:])

                elif tags[0] == 'RECV':
                    cardid = int(tags[1])
                    self.conn.lock.acquire()
                    if tags[2] == 'DATA':
                        if tags[3][0:2] == '81':
                            self.conn.input_data[cardid] = tags[3]
                        elif tags[3][0:2] == '01':
                            self.conn.output_data[cardid] = tags[3]
                        elif tags[3][0:2] == '03':
                            self.conn.config1_data[cardid] = tags[3]
                        else:
                            print '_reader(): got', line
                            print 'don\'t know what to do with that'
                            sys.exit(1)
                    else:
                        print '_reader(): got', line
                        print 'don\'t know what to do with that'

                    if cardid in self.conn.cardfifo:
                        self.conn.cardfifo[cardid].put(tags)
                    self.conn.lock.release()

                elif tags[0] == 'OPEN':
                    cardid = int(tags[1])
                    self.conn.lock.acquire()
                    if cardid in self.conn.cardfifo:
                        self.conn.cardfifo[cardid].put(tags)
                    self.conn.lock.release()

                elif tags[0] == 'BYE':
                    break

                else:
                    print '_reader(): got', line
                    print 'don\'t know what to do with that'
                    sys.exit(1)


# ----------
# connect()
#
#   Global function to create a connected Open8055 object.
# ----------
def connect(host='localhost', port=8055, timeout=None,
        user='nobody', password=''):
    conn = Open8055()
    conn.connect(host, port, timeout, user, password)
    return conn


