# ----------------------------------------------------------------------
# open8055.py
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



class Open8055:
    def __init__(self):
        self.sock           = None
        self.fdin           = None
        self.cardfifo       = {}
        self.autosend_flag  = {}
        self.output_data    = {}
        self.input_data     = {}
        self.config1_data   = {}

    # ----------
    # connect()
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
    # ----------
    def disconnect(self):
        if self.fdin:
            self.sock.sendall('BYE\n');
            self.reader.join()

            self.fdin.close()
            self.fdin = None
            self.sock = None


    # ----------
    # list()
    # ----------
    def list(self):
        self.sock.sendall('LIST\n')
        reply = self.listfifo.get()
        if reply[0] == 'OK':
            result = []
            for cardid in reply[1:]:
                result.append(int(cardid))
            return result
        raise Exception(' '.join(result[1:]))


    # ----------
    # open()
    # ----------
    def open(self, cardid):
        self.cardfifo[cardid] = Queue.Queue()

        # ----
        # First we send the OPEN command and expect 'OPEN cardid OK'
        # ----
        self.sock.sendall('OPEN ' + str(cardid) + '\n')
        result = self.cardfifo[cardid].get()
        if result != ['OPEN', str(cardid), 'OK']:
            del self.cardfifo[cardid]
            raise Exception(' '.join(result[3:]))

        # ----
        # Now send the GETCONFIG command and wait for all messages
        # we expect in return.
        # ----
        self.sock.sendall('SEND ' + str(cardid) + ' 04\n')
        have_input      = False
        have_output     = False
        have_config1    = False

        while not have_input or not have_output or not have_config1:
            result = self.cardfifo[cardid].get()

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
                raise Exception(' '.join(result))

        # ----
        # Stop listening for messages on the card fifo. User
        # can turn this on/off as needed later.
        # ----
        del self.cardfifo[cardid]

        # ----
        # Set the card to autosend mode.
        # ----
        self.autosend_flag[cardid] = True


    def autosend(self, cardid, mode=None):
        ret = self.autosend_flag[cardid]
        if mode != None:
            self.autosend_flag[cardid] = mode
        return ret

    def get_input_all(self, cardid):
        return int(self.input_data[cardid][2:4], 16)

    def get_input_port(self, cardid, port):
        if port < 0 or port > 4:
            return 0
        if self.get_input_all(cardid) & (1 << port):
            return 1
        return 0

    def get_output_all(self, cardid):
        return int(self.output_data[cardid][2:4], 16)

    def get_output_port(self, cardid, port):
        if port < 0 or port > 7:
            return 0
        if self.get_output_all(cardid) & (1 << port):
            return 1
        return 0
        
    def set_output_all(self, cardid, val):
        self.output_data[cardid] = '01{0:02X}'.format(val & 0xFF) + \
                self.output_data[cardid][4:]
        if self.autosend_flag[cardid]:
            self.send(cardid)

    def set_output_port(self, cardid, port, val):
        if port < 0 or port > 7:
            return

        out = self.get_output_all(cardid)
        if val:
            out |= (1 << port)
        else:
            out &= ~(1 << port)

        self.set_output_all(cardid, out)

    def send(self, cardid):
       self.sock.sendall('SEND ' + str(cardid) + ' ' + \
            self.output_data[cardid] + '\n')


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
                    if cardid in self.conn.cardfifo:
                        self.conn.cardfifo[cardid].put(tags)

                elif tags[0] == 'OPEN':
                    cardid = int(tags[1])
                    if cardid in self.conn.cardfifo:
                        self.conn.cardfifo[cardid].put(tags)

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


