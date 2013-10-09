#!/usr/bin/env python

import ConfigParser
import hashlib
import netaddr
import os
import random
import re
import select
import socket
import struct
import sys
import threading
import time

import open8055io

if os.name == 'posix':
    import signal
elif os.name == 'nt':
    import win32service
    import win32serviceutil
    import win32api
    import win32con
    import win32event
    import win32evtlogutil
else:
    raise Exception('unsupported OS name "' + os.name + '"')

MODE_RUN = 1
MODE_STOP = 2
MODE_STOPPED = 3

# ----------------------------------------------------------------------
# Open8055Server
# ----------------------------------------------------------------------
class Open8055Server(threading.Thread):
    """
    Implementation of the Open8055 TCP/IP server process or service.
    """

    SERVERNAME = 'Open8055Server'
    VERSION = '0.1.0'

    MAX_CARDS = 16

    # ----------
    # __init__()
    #
    #   Initialize some status data and create the server socket.
    # ----------
    def __init__(self):
        threading.Thread.__init__(self)

        self.status = MODE_RUN
        self.lock = threading.Lock()
        self.clients = []

    def create_server_socket(self):
        # ----
        # Create the server socket.
        # ----
        port = self.config.getint('General', 'server_port')
        self.sock = None
        if socket.has_ipv6:
            try:
                self.sock = socket.socket(
                        socket.AF_INET6, socket.SOCK_STREAM, 0)
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                self.sock.bind(('', port))
                self.sock.listen(10)
            except:
                self.sock = None
                pass
        if not self.sock:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.bind(('', port))
            self.sock.listen(10)

    # ----
    # load_config()
    # ----
    def load_config(self, config_locations, is_reload = False):
        self.config = ConfigParser.SafeConfigParser()

        self.config.add_section('General')
        self.config.set('General', 'server_port', '8055')
        self.config.set('General', 'users_file', 'open8055.users')

        self.config.add_section('Access')
        self.config.set('Access', 'connect', """127.0.0.1/32    all     trust
                        ::1/128     all     trust
                        0.0.0.0/0   all     deny
                        ::/0        all     deny""")
        self.config.set('Access', 'default', """127.0.0.1/32    all     trust
                        ::1/128     all     trust
                        0.0.0.0/0   all     deny
                        ::/0        all     deny""")

        if not is_reload:
            self.config_fname = None
            for fname in config_locations:
                if not os.path.exists(fname):
                    continue
                try:
                    self.config.read(fname)
                except Exception as err:
                    log_error('cannot load {0}: {1}'.format(fname, str(err)))
                    continue
                
                log_info('configuration loaded from {0}'.format(fname))
                self.config_fname = fname
                break
        else:
            if self.config_fname is not None:
                self.config.read(self.config_fname)
                

    # ----------
    # run()
    # ----------
    def run(self):
        while True:
            self.lock.acquire()
            if self.status != MODE_RUN:
                # ----
                # On STOP notify all existing clients to close and
                # wait for them to finish.
                # ----
                for client in self.clients:
                    client.set_status(MODE_STOP)

                for client in self.clients:
                    client.join()

                # ----
                # Close the server socket.
                # ----
                self.sock.shutdown(socket.SHUT_RDWR)
                self.sock.close()

                # ----
                # Finally set the status to STOPPED and end this thread.
                # ----
                self.status = MODE_STOPPED
                self.lock.release()
                return
            self.lock.release()

            # ----
            # We are still in RUN mode. Wait for a new client to connect.
            # ----
            try:
                rdy, _dummy, _dummy = select.select((self.sock,), (), (), 2.0)
            except Exception as err:
                log_error('select() on server socket failed:' + str(err))
                self.lock.acquire()
                self.status = MODE_STOPPED
                self.lock.release()
                return

            # ----
            # Whenever we get a new client connection or the select() has
            # timed out we try to clean up after terminated clients.
            # ----
            self.reaper()

            # ----
            # If no client connected, just loop.
            # ----
            if len(rdy) == 0:
                continue

            # ----
            # Accept new client connection.
            # ----
            try:
                conn, addr = self.sock.accept()
            except Exception as err:
                log_error('accept() on server socket failed:' + str(err))
                self.lock.acquire()
                self.status = MODE_STOPPED
                self.lock.release()
                return

            # ----
            # Check the client address against the [Access] config section.
            # ----
            if not self.check_connect_access(addr):
                log_error('client {0}: connect denied'.format(addr))
                try:
                    conn.send('ERROR access denied\n')
                    conn.shutdown(socket.SHUT_RDWR)
                    conn.close()
                    continue
                except:
                    continue

            # ----
            # Create a client thread for it and add it to the list.
            # ----
            client = Open8055Client(self, conn, addr)
            self.clients.append(client)
            client.start()

    # ----------
    # check_connect_access()
    #
    #   Compare the IP address of the client against the [Access] section
    #   of the config file.
    # ----------
    def check_connect_access(self, addr):
        try:
            result = self.lookup_auth_method(addr[0], None,
                    self.config.get('Access', 'connect'))
        except Exception as err:
            log_error('lookup_auth_method() failed: ' + str(err))
            return False
        if result in ('trust', 'plain', 'md5', ):
            return True
        return False

    # ----------
    # check_list_access()
    #
    #   Compare the address, user and password against the "connect"
    #   Access entry in the config file, and eventually check the
    #   username/password given.
    # ----------
    def check_list_access(self, addr, user, password, salt):
        # ----
        # Get the required authentication method based on client address.
        # ----
        try:
            auth_required = self.lookup_auth_method(addr[0], user,
                    self.config.get('Access', 'connect'))
        except Exception as err:
            log_error('lookup_auth_method() failed: ' + str(err))
            return False

        # ----
        # If this is denied based on user or IP, return False.
        # ----
        if auth_required == 'deny':
            return False

        # ----
        # 'trust' authentication means we accept any username and password.
        # ----
        if auth_required == 'trust':
            return True
        
        # ----
        # Need to check username and password and see if we got the
        # required password level. A given md5 password satisfies both,
        # md5 or plain requirement, but not the other way around.
        # ----
        auth_ok, auth_issuper, auth_given = self.check_user_password(
                user, password, salt)
        if not auth_ok:
            return False
        if auth_required == 'md5' and auth_given != 'md5':
            log_error('client {0}: md5 authentication required, got {1}'.format(
                    addr, auth_given))
            return False
        return True

    # ----------
    # check_open_access()
    #
    #   Compare the address, user and password against the cards
    #   Access entry in the config file, and eventually check the
    #   username/password given.
    # ----------
    def check_open_access(self, cardid, addr, user, password, salt):
        # ----
        # Get the required authentication method based on client address.
        # ----
        if self.config.has_option('Access', 'card_' + str(cardid)):
            access_list = 'card_' + str(cardid)
        else:
            access_list = 'default'

        try:
            auth_required = self.lookup_auth_method(addr[0], user,
                    self.config.get('Access', access_list))
        except Exception as err:
            log_error('lookup_auth_method() failed: ' + str(err))
            return False

        # ----
        # If this is denied based on user or IP, return False.
        # ----
        if auth_required == 'deny':
            return False

        # ----
        # 'trust' authentication means we accept any username and password.
        # ----
        if auth_required == 'trust':
            return True
        
        # ----
        # Need to check username and password and see if we got the
        # required password level. A given md5 password satisfies both,
        # md5 or plain requirement, but not the other way around.
        # ----
        auth_ok, auth_issuper, auth_given = self.check_user_password(
                user, password, salt)
        if not auth_ok:
            return False
        if auth_required == 'md5' and auth_given != 'md5':
            log_error('client {0}: md5 authentication required, got {1}'.format(
                    addr, auth_given))
            return False
        return True

    # ----
    # check_user_password()
    # ----
    def check_user_password(self, user, password, salt):
        try:
            # ----
            # Locate the users file
            # ----
            fname = self.config.get('General', 'users_file')
            if not os.path.isabs(fname):
                if self.config_fname is not None:
                    fname = os.path.realpath(os.path.join(
                            os.path.dirname(
                            os.path.realpath(self.config_fname)), fname))
                else:
                    fname = os.path.realpath(os.path.join(
                            os.path.dirname(os.path.realpath(__file__)), fname))

            # ----
            # Read the users file.
            # ----
            fd = open(fname, 'r')
            while True:
                line = fd.readline()
                if len(line) == 0:
                    break
                auth_user, auth_issuper, auth_password = line.strip().split(':')
                if auth_user != user:
                    continue
                if len(auth_password) != 35 or auth_password[0:3] != 'md5':
                    auth_password = 'md5' + hashlib.md5(
                            auth_password).hexdigest()
                # ----
                # This is the user. If the given password starts with 'md5'
                # and is 35 characters long, we expect it to be the md5 hash
                # of the SALT and the hashed real user password (double hash).
                # This is done to prevent password repeat attacks.
                # ----
                if len(password) == 35 and password[0:3] == 'md5':
                    salt_password = hashlib.md5(
                            salt + auth_password[3:]).hexdigest()
                    if password[3:] == salt_password:
                        # ----
                        # That matched. This is a double hashed md5.
                        # ----
                        fd.close()
                        return True, bool(auth_issuper), 'md5'
                # ----
                # This is certainly not a double hashed md5. Test if it
                # is a plain password.
                # ----
                if hashlib.md5(password).hexdigest() == auth_password[3:]:
                    # ----
                    # This matched, but is only 'plain' authentication.
                    # ----
                    fd.close()
                    return True, bool(auth_issuper), 'plain'

                # ----
                # Password mismatch
                # ----
                fd.close()
                return False, False, 'none'

            # ----
            # User not found in users file.
            # ----
            fd.close()
            return False, False, 'none'
        except Exception as err:
            log_error('{0}: {1}'.format(
                    self.config.get('General', 'users_file'), err))
            return False, False, 'none'
            
    # ----------
    # lookup_auth_method()
    #
    #   Try to match the give IP address to the narrowest network or
    #   host address in the access list. 
    # ----------
    def lookup_auth_method(self, ipaddr, user, access_list):
        # ----
        # We work everything IPV6 mapped
        # ----
        ipaddr = netaddr.IPAddress(ipaddr).ipv6()

        re_comment = re.compile('^[ \t]*[#;]')
        re_lines = re.compile('[ \t\r]*\n[ \t\r]*')
        re_blank = re.compile('[ \t]+')

        # ----
        # Process the access list line by line, stop at the first match.
        # ----
        for line in re_lines.split(access_list.strip()):
            # ----
            # Ignore empty lines and comments.
            # ----
            if len(line) == 0:
                continue
            if re_comment.match(line):
                continue
            
            # ----
            # Split the line into network address, auth-user  and auth-result.
            # Return the result if the ipaddr falls into the network.
            # ----
            network, auth_user, result = re_blank.split(line)
            network = netaddr.IPNetwork(network).ipv6()
            if user is not None and auth_user != 'all':
                if user != auth_user:
                    continue

            if len(netaddr.all_matching_cidrs(ipaddr, (network, ))) > 0:
                return result

        # ----
        # No match at all? This should not happen. The detault config
        # contains "deny" lines for INADDR_ANY and its IPV6 counterpart.
        # ----
        return 'deny'
        
    # ----------
    # reaper()
    #
    #   Check if any client threads have ended. Join them and
    #   remove them from the list of clients.
    # ----------
    def reaper(self):
        clients = []
        for client in self.clients:
            if client.get_status() == MODE_STOPPED:
                client.join()
            else:
                clients.append(client)
        self.clients = clients

    # ----------
    # get_status()
    #
    #   Get the current run status of the server.
    # ----------
    def get_status(self):
        self.lock.acquire()
        ret = self.status
        self.lock.release()
        return ret

    # ----------
    # shutdown()
    #
    #   Set the run status of the server to STOP and wait for
    #   the thread accepting client connections to end. This
    #   also implies that all client connections have been
    #   terminated.
    # ----------
    def shutdown(self):
        self.lock.acquire()
        self.status = MODE_STOP
        self.lock.release()
        self.join()

        
# ----------------------------------------------------------------------
# Open8055Client
#
#   Class implementing the thread handling one client connection.
# ----------------------------------------------------------------------
class Open8055Client(threading.Thread):
    # ----------
    # __init__()
    # ----------
    def __init__(self, server, conn, addr):
        threading.Thread.__init__(self)

        self.server = server
        self.status = MODE_RUN

        self.lock = threading.Lock()
        self.conn = conn
        self.inbuf = ''
        self.addr = addr

        self.user = None
        self.salt = '{0:016x}'.format(random.getrandbits(64))

        self.cardid = -1
        self.cardio = None

    # ----------
    # run()
    # ----------
    def run(self):
        # ----
        # Send HELLO and SALT messages to new client.
        # ----
        try:
            self.send('HELLO {0} {1}\n'.format(
                Open8055Server.SERVERNAME, Open8055Server.VERSION))
            self.send('SALT ' + self.salt + '\n')
        except Exception as err:
            log_error('client {0}: {1}'.format(str(self.addr), str(err)))
            self.set_status(MODE_STOPPED)
            return

        # ----
        # Run until the main server thread tells us to STOP or
        # the remote disconnects.
        # ----
        while self.get_status() == MODE_RUN:
            # ----
            # See if we still have another command in the input buffer.
            # ----
            idx = self.inbuf.find('\n')
            if idx < 0:
                # ----
                # No NEWLINE in there, wait for more data.
                # ----
                try:
                    rdy, _dummy, _dummy = select.select(
                            (self.conn,), (), (), 2.0)
                except Exception as err:
                    log_error('client {0}: {1}'.format(
                            str(self.addr), str(err)))
                    break

                if self.cardio:
                    if self.cardio.get_status() == MODE_STOPPED:
                        log_error('client {0}: {1}'.format(
                                str(self.addr), 'cardio stopped unexpected'))
                        break

                # ----
                # If rdy is empty then this was just a timeout to check
                # for STOP flag.
                # ----
                if len(rdy) == 0:
                    continue

                # ----
                # Receive new data
                # ----
                try:
                    data = self.conn.recv(256)
                except Exception as err:
                    log_error('client {0}: {1}'.format(
                            str(self.addr), str(err)))
                    break

                # ----
                # Check of EOF
                # ----
                if len(data) == 0:
                    break
                
                # ----
                # Add the data to the input buffer. If there's still
                # no NEWLINE, wait for more.
                # ----
                self.inbuf += data
                idx = self.inbuf.find('\n')
                if idx < 0:
                    continue

            # ----
            # We have a NEWLINE in the input buffer. Consume the
            # first line.
            # ----
            line = self.inbuf[0:idx]
            self.inbuf = self.inbuf[idx + 1:]

            # ----
            # Split the command line by spaces and process it.
            # ----
            args = line.strip().split(' ')

            try:
                if args[0].upper() == 'SEND':
                    self.cmd_send(args)

                elif args[0].upper() == 'LIST':
                    self.cmd_list(args)

                elif args[0].upper() == 'OPEN':
                    self.cmd_open(args)

                elif args[0].upper() == 'QUIT':
                    self.set_status(MODE_STOP)
                    break

                else:
                    self.send('ERROR unknown command \'' +
                            args[0].upper() + '\'\n')

            except Exception as err:
                log_error('client {0}: {1}'.format(str(self.addr), str(err)))
                try:
                    self.send('ERROR ' + str(err) + '\n')
                except:
                    pass

        # ----
        # Stop the reader thread if one exists.
        # ----
        if self.cardio is not None:
            try:
                if self.cardio.get_status() != MODE_STOPPED:
                    self.cardio.set_status(MODE_STOP)
                    try:
                        open8055io.write(self.cardid, struct.pack('B', 0x02))
                    except Exception as err:
                        log_error('client {0}: {1}'.format(
                                str(self.addr), str(err)))
                self.cardio.join()
            except Exception as err:
                log_error('client {0}: {1}'.format(str(self.addr), str(err)))
                try:
                    self.send('ERROR ' + str(err))
                except:
                    pass

        # ----
        # Close the Open8055
        # ----
        try:
            if self.cardid >= 0:
                open8055io.close(self.cardid)
        except Exception as err:
            log_error('client {0}: {1}'.format(self.addr, str(err)))

        # ----
        # Close the remote connection.
        # ----
        try:
            if self.conn is not None:
                self.conn.shutdown(socket.SHUT_RDWR)
                self.conn.close()
        except Exception as err:
            log_error('client {0}: {1}'.format(self.addr, str(err)))

        # ----
        # Set the run status to STOPPED and end this thread.
        # ----
        self.set_status(MODE_STOPPED)
        return

    # ----------
    # cmd_list()
    #
    #   Reports available cards
    # ----------
    def cmd_list(self, args):
        if len(args) != 3:
            raise Exception('usage: LIST username password')

        allowed = self.server.check_list_access(self.addr, 
                args[1], args[2], self.salt)
        if not allowed:
            log_error('client {0}: LIST {1} ***** - permission denied'.format(
                    self.addr, args[1]))
            self.send('ERROR permission denied\n')
            return

        response = 'LIST'
        for cardid in range(0, Open8055Server.MAX_CARDS):
            if open8055io.present(cardid):
                response += ' ' + str(cardid)
        self.send(response + '\n')

    # ----------
    # cmd_open()
    # ----------
    def cmd_open(self, args):
        if len(args) != 4:
            raise Exception('usage: OPEN cardid username password')
        if self.cardid >= 0:
            raise Exception('already connected to card ' + str(self.cardid))

        cardid = int(args[1])

        allowed = self.server.check_open_access(cardid, self.addr, 
                args[2], args[3], self.salt)
        if not allowed:
            log_error('client {0}: OPEN {1} {2} ***** - permission denied'.format(
                    self.addr, args[1], args[2]))
            self.send('ERROR permission denied\n')
            return

        open8055io.open(cardid)

        cardio = Open8055Reader(self, cardid)
        cardio.start()

        self.cardid = cardid
        self.cardio = cardio

        # ----
        # We send a GETCONFIG message to the card and the reader
        # is going to suppress INPUT messages until OUTPUT and CONFIG1
        # have been reported.
        # ----
        open8055io.write(cardid, struct.pack('B', 0x04))

    # ----------
    # cmd_send()
    # ----------
    def cmd_send(self, args):
        if self.cardid < 0:
            raise Exception('not connected to a card')

        # ----
        # Get the HID command message format by type
        # ----
        hid_type = int(args[1])
        if hid_type == 0x01:            # OUTPUT
            msg_fmt = '!BB8H2HB'
            num_val = 13
        elif hid_type == 0x02:          # GETINPUT
            msg_fmt = '!B'
            num_val = 1
        elif hid_type == 0x03:          # SETCONFIG1
            msg_fmt = '!B2B5B8B2B5HB'
            num_val = 24
        elif hid_type == 0x04:          # GETCONFIG
            msg_fmt = '!B'
            num_val = 1
        elif hid_type == 0x05:          # SAVECONFIG
            msg_fmt = '!B'
            num_val = 1
        elif hid_type == 0x06:          # SAVEALL
            msg_fmt = '!B'
            num_val = 1
        elif hid_type == 0x7F:          # RESET
            log_info('client {0} sent RESET command'.format(self.addr))
            msg_fmt = '!B'
            num_val = 1
        else:
            raise Exception('invalid HID command type 0x{0:02X}'.format(
                    hid_type))


        # ----
        # Create a sequence of integers for that format. Add zeroes
        # for missing members at the end (telnet debugging aid)
        # ----
        vals = [int(x) for x in args[1:]]
        while len(vals) < num_val:
            vals.append(0)

        # ----
        # Pack this into the binary message and send it to the card.
        # ----
        data = struct.pack(msg_fmt, *vals)

        try:
            open8055io.write(self.cardid, data)
        except Exception as err:
            self.set_status(MODE_STOP)
            try:
                self.client.send('ERROR from write ' + str(err) + '\n')
            except:
                pass

    # ----------
    # send()
    #
    #   Send one message to the remote client. If that fails, log the
    #   error, close the connection and re-raise the exception.
    # ----------
    def send(self, msg):
        self.lock.acquire()
        try:
            if self.conn:
                self.conn.sendall(msg)
        except Exception as err:
            log_error('client {0}: {1}'.format(str(self.addr), str(err)))
            try:
                self.conn.shutdown(socket.SHUT_RDWR)
                self.conn.close()
            except:
                pass
            self.conn = None
            self.lock.release()
            raise err

        self.lock.release()

    # ----------
    # get_status()
    # ----------
    def get_status(self):
        self.lock.acquire()
        ret = self.status
        self.lock.release()
        return ret

    # ----------
    # get_status()
    # ----------
    def set_status(self, status):
        self.lock.acquire()
        self.status = status
        self.lock.release()


# ----------------------------------------------------------------------
# Open8055Reader
#
#   Class implementing a thread that reads data from an Open8055 card
#   and sends it to the associated client.
# ----------------------------------------------------------------------
class Open8055Reader(threading.Thread):
    def __init__(self, client, cardid):
        threading.Thread.__init__(self)

        self.client = client
        self.cardid = cardid
        self.startup = True
        self.had_config1 = False
        self.had_output = False
        self.lock = threading.Lock()
        self.status = MODE_RUN

    def run(self):
        while self.get_status() == MODE_RUN:
            try:
                data = open8055io.read(self.cardid)
            except Exception as err:
                try:
                    log_error('client {0}: {1}'.format(
                            str(self.client.addr), str(err)))
                    self.client.send('ERROR ' + str(err) + '\n')
                    break
                except:
                    pass

            # ----
            # In client startup mode we suppress all messages until
            # we sent the CONFIG1 and OUTPUT messages.
            # ----
            hid_type = ord(data[0])
            if self.startup:
                if hid_type == 0x03:
                    self.had_config1 = True
                elif hid_type == 0x01:
                    self.had_output = True
                else:
                    if self.had_output and self.had_config1:
                        self.startup = False
                    else:
                        continue

            # ----
            # Format the client message according to the report type.
            # ----
            if hid_type == 0x81:
                msg_fmt = '!BB5H2H'
            elif hid_type == 0x01:
                msg_fmt = '!BB8H2HB'
            elif hid_type == 0x03:
                msg_fmt = '!B2B5B8B2B5HB'
            else:
                try:
                    self.client.send('ERROR unknown HID packet type ' +
                            '0x{0:02X} received from card'.format(hid_type))
                except:
                    pass
                break

            message = ' '.join(str(elem) for elem in 
                    struct.unpack(msg_fmt, data[0:struct.calcsize(msg_fmt)]))

            try:
                self.client.send('RECV ' + message + '\n')
            except Exception as err:
                log_error(str(err))
                break
            
        # ----
        # The reader loop exited. Terminate this thread.
        # ----
        self.set_status(MODE_STOPPED)
        return

    def get_status(self):
        #self.lock.acquire()
        ret = self.status
        #self.lock.release()
        return ret

    def set_status(self, status):
        #self.lock.acquire()
        self.status = status
        #self.lock.release()


# ----------------------------------------------------------------------
# Posix specific watchdog and startup code
# ----------------------------------------------------------------------
if os.name == 'posix':
    def main(argv):
        # ----
        # Initially ignore signals. This will be inherited by
        # the actual server process.
        # ----
        signal.signal(signal.SIGINT, signal.SIG_IGN)
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        if os.name == 'posix':
            signal.signal(signal.SIGHUP, signal.SIG_IGN)

        # ----
        # We create a pipe. When the watchdog process closes the writing
        # end it signals the server process to shutdown.
        # ----
        p_rd, p_wr = os.pipe()

        # ----
        # Create the server process via fork()
        # ---
        server_pid = os.fork()
        if server_pid == 0:
            # ----
            # This is the server process. We need to close the
            # writing end of the pipe or we never see EOF on it.
            # ----
            os.close(p_wr)

            # ----
            # Create the Open8055 server
            # ----
            try:
                server = Open8055Server()

                config_locations = ['/usr/local/etc/open8055.conf']
                config_locations.append(os.path.join(os.path.dirname(
                        os.path.realpath(__file__)), 'open8055.conf'))
                server.load_config(config_locations)

                server.create_server_socket()

                server.start()
            except Exception as err:
                log_error('Open8055server failed: ' + str(err))
                sys.exit(2)
            
            # ----
            # Wait until either the server stopped on its own due to some
            # internal error, or the watchdog tells us to shutdown.
            # ----
            while server.get_status() != MODE_STOPPED:
                try:
                    rdy, _dummy, _dummy = select.select((p_rd,), (), (), 2.0)
                except Exception as err:
                    log_error('select() failed: ' + str(err))
                    sys.exit(3)
                
                if len(rdy) != 0:
                    server.shutdown()
                    break

            sys.exit(0)
            
        # ----
        # This is the watchdog process after fork().
        # Close the reading end of the pipe, used by the server process.
        # ----
        os.close(p_rd)

        # ----
        # Catch signals.
        # ----
        signal.signal(signal.SIGINT, main_catch_signal)
        signal.signal(signal.SIGTERM, main_catch_signal)

        try:
            wpid, status = os.wait()
        except:
            os.close(p_wr)
            wpid, status = os.wait()

        return status


    def main_catch_signal(signum, frame):
        pass


    def log_info(message):
        print message


    def log_error(message):
        print message


    if __name__ == '__main__':
        sys.exit(main(sys.argv))

# ----------------------------------------------------------------------
# Windows specific service code
# ----------------------------------------------------------------------
elif os.name == 'nt':
    import servicemanager
    
    class open8055server(win32serviceutil.ServiceFramework):
        _svc_name_ = 'open8055server'
        _svc_display_name_ = 'Open8055Server'
        _svc_description_ = 'TCP/IP server for Open8055 USB cards'

        def __init__(self, args):
            win32serviceutil.ServiceFramework.__init__(self, args)
            self.stopEvent = win32event.CreateEvent(None, 0, 0, None)

        def SvcStop(self):
            self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
            win32event.SetEvent(self.stopEvent)
    
        def SvcDoRun(self):
            servicemanager.LogMsg(servicemanager.EVENTLOG_INFORMATION_TYPE,
                    servicemanager.PYS_SERVICE_STARTED,
                    (self._svc_name_, ''))

            server = Open8055Server()

            config_locations = [os.path.join(os.path.dirname(__file__),
                    'open8055.conf')]
            server.load_config(config_locations)

            server.create_server_socket()

            server.start()

            while True:
                rc = win32event.WaitForSingleObject(self.stopEvent, 5000)

                if rc == win32event.WAIT_OBJECT_0:
                    server.shutdown()
                    servicemanager.LogInfoMsg('open8055server - STOPPED')
                    break
                else:
                    if server.get_status() == MODE_STOPPED:
                        servicemanager.LogErrorMsg('open8055server - CRASHED')
                        break

    def log_info(msg):
        servicemanager.LogInfoMsg('open8055server - ' + msg)


    def log_error(msg):
        servicemanager.LogErrorMsg('open8055server - ' + msg)


    def ctrlHandler(ctrlType):
        return True

    if __name__ == '__main__':
        win32api.SetConsoleCtrlHandler(ctrlHandler, True)
        win32serviceutil.HandleCommandLine(open8055server)
