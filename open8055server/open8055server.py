#!/usr/bin/env python

import os, sys, time, threading
import socket, select, random
import open8055io

if os.name == 'posix':
    import signal
elif os.name == 'nt':
    import win32service, win32serviceutil
    import win32api, win32con
    import win32event, win32evtlogutil
else:
    raise Exception('unsupported OS name "' + os.name + '"')

# ----------------------------------------------------------------------
# Open8055Server
#
#   Class implementing a thread that accepts new client connections
#   and creates a Open8055Client thread for each of them.
# ----------------------------------------------------------------------
class Open8055Server(threading.Thread):
    SERVERNAME  = 'Open8055Server'
    VERSION     = '0.1.0'

    MAX_CARDS   = 16

    # ----------
    # __init__()
    #
    #   Initialize some status data and create the server socket.
    # ----------
    def __init__(self, port):
        threading.Thread.__init__(self)

        self.status     = 'RUN'
        self.lock       = threading.Lock()
        self.clients    = []

        # ----
        # Create the server socket.
        # ----
        self.sock = None
        if socket.has_ipv6:
            try:
                self.sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM, 0)
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

    # ----------
    # run()
    # ----------
    def run(self):
        while True:
            self.lock.acquire()
            if self.status != 'RUN':
                # ----
                # On STOP notify all existing clients to close and
                # wait for them to finish.
                # ----
                for client in self.clients:
                    client.set_status('STOP')

                for client in self.clients:
                    client.join()

                # ----
                # Close the server socket.
                # ----
                self.sock.close()

                # ----
                # Finally set the status to STOPPED and end this thread.
                # ----
                self.status = 'STOPPED'
                self.lock.release()
                return
            self.lock.release()

            # ----
            # We are still in RUN mode. Wait for a new client to connect.
            # ----
            try:
                rdy, _dummy, _dummy = select.select((self.sock,), (), (), 1.0)
            except Exception as err:
                log('select() on server socket failed:' + str(err))
                self.lock.acquire()
                self.status = 'STOPPED'
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
                log('accept() on server socket failed:' + str(err))
                self.lock.acquire()
                self.status = 'STOPPED'
                self.lock.release()
                return

            # ----
            # Create a client thread for it and add it to the list.
            # ----
            client = Open8055Client(conn, addr)
            self.clients.append(client)
            client.start()
            
    # ----------
    # reaper()
    #
    #   Check if any client threads have ended. Join them and
    #   remove them from the list of clients.
    # ----------
    def reaper(self):
        clients = []
        for client in self.clients:
            if client.get_status() == 'STOPPED':
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
        self.status = 'STOP'
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
    def __init__(self, conn, addr):
        threading.Thread.__init__(self)

        self.status = 'RUN'

        self.lock   = threading.Lock()
        self.conn   = conn
        self.rfile  = conn.makefile('r')
        self.addr   = addr

        self.user   = None
        self.salt   = '{0:016X}'.format(random.getrandbits(64))

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
            self.set_status('STOPPED')
            return

        # ----
        # Run until the main server thread tells us to STOP or
        # the remote disconnects.
        # ----
        while self.get_status() == 'RUN':
            try:
                rdy, _dummy, _dummy = select.select((self.conn,), (), (), 1.0)
            except Exception as err:
                log('client {0}: {1}'.format(str(self.addr), str(err)))
                break

            if self.cardio:
                if self.cardio.get_status() == 'STOPPED':
                    break

            # ----
            # If rdy is empty then this was just a timeout to check
            # for STOP flag.
            # ----
            if len(rdy) == 0:
                continue

            # ----
            # Receive and process the command
            # ----
            try:
                data = self.rfile.readline()

                # ----
                # Check of EOF
                # ----
                if not data:
                    break

                # ----
                # Split the command line by spaces and process it.
                # ----
                args = data.strip().split(' ')

                try:
                    if args[0].upper() == 'SEND':
                        self.cmd_send(args)

                    elif args[0].upper() == 'LIST':
                        self.cmd_list(args)

                    elif args[0].upper() == 'OPEN':
                        self.cmd_open(args)

                    elif args[0].upper() == 'QUIT':
                        break

                    else:
                        self.send('ERROR unknown command \'' +
                                args[0].upper() + '\'\n')

                except Exception as err:
                    try:
                        self.send('ERROR ' + str(err) + '\n')
                    except:
                        pass

            except Exception as err:
                log('client {0}: {1}'.format(str(self.addr), str(err)))
                break

        # ----
        # Stop the reader thread if one exists.
        # ----
        if self.cardio:
            try:
                self.cardio.set_status('STOP')
                try:
                    open8055io.write(self.cardid, '81')
                except:
                    pass
                self.cardio.join()
            except Exception as err:
                try:
                    self.send('ERROR ' + str(err))
                except:
                    pass

        # ----
        # Close the remote connection.
        # ----
        try:
            if self.conn:
                self.conn.close()
        except:
            pass

        try:
            if self.rfile:
                self.rfile.close()
        except:
            pass

        # ----
        # Set the run status to STOPPED and end this thread.
        # ----
        self.set_status('STOPPED')
        return

    # ----------
    # cmd_list()
    #
    #   Reports available cards
    # ----------
    def cmd_list(self, args):
        if len(args) != 3:
            raise Exception('usage: LIST username password')

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

        open8055io.open(cardid)

        cardio = Open8055Reader(self, cardid)
        cardio.start()

        self.cardid = cardid
        self.cardio = cardio

        try:
            self.send('OK\n')
        except:
            self.set_status('STOP')

    # ----------
    # cmd_send()
    # ----------
    def cmd_send(self, args):
        if len(args) != 2:
            raise Exception('usage: SEND data')
        if self.cardid < 0:
            raise Exception('not connected to a card')

        try:
            open8055io.write(self.cardid, args[1])
        except Exception as err:
            self.set_status('STOP')
            try:
                self.client.send('ERROR ' + str(err) + '\n')
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
            log('client {0}: {1}'.format(str(self.addr), str(err)))
            try:
                self.conn.close()
            except:
                pass
            self.conn = None
            try:
                self.rfile.close()
            except:
                pass
            self.rfile = None
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
# ----------------------------------------------------------------------
class Open8055Reader(threading.Thread):
    def __init__(self, client, cardid):
        threading.Thread.__init__(self)

        self.client = client
        self.cardid = cardid
        self.lock   = threading.Lock()
        self.status = 'RUN'

    def run(self):
        try:
            while self.get_status() == 'RUN':
                data = open8055io.read(self.cardid)
                self.client.send('RECV ' + data + '\n')
            open8055io.close(self.cardid)
        except Exception as err:
            try:
                self.client.send('ERROR ' + str(err) + '\n')
            except:
                pass

            try:
                open8055io.close(self.cardid)
            except:
                pass

        self.set_status('STOPPED')
        return

    def get_status(self):
        self.lock.acquire()
        ret = self.status
        self.lock.release()
        return ret

    def set_status(self, status):
        self.lock.acquire()
        self.status = status
        self.lock.release()


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
                server = Open8055Server(8055)
                server.start()
            except Exception as err:
                log('Open8055server failed: ' + str(err))
                sys.exit(2)
            
            # ----
            # Wait until either the server stopped on its own due to some
            # internal error, or the watchdog tells us to shutdown.
            # ----
            while server.get_status() != 'STOPPED':
                try:
                    rdy, _dummy, _dummy = select.select((p_rd,), (), (), 5.0)
                except Exception as err:
                    log('select() failed: ' + str(err))
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


    def log(message):
        print message


    if __name__ == '__main__':
        sys.exit(main(sys.argv))

# ----------------------------------------------------------------------
# Windows specific service code
# ----------------------------------------------------------------------
elif os.name == 'nt':
    class open8055server(win32serviceutil.ServiceFramework):
        _svc_name_          = 'open8055server'
        _svc_display_name_  = 'Open8055Server'
        _svc_description_   = 'TCP/IP server for Open8055 USB cards'

        def __init__(self, args):
            win32serviceutil.ServiceFramework.__init__(self, args)
            self.stopEvent = win32event.CreateEvent(None, 0, 0, None)

        def SvcStop(self):
            self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)
            win32event.SetEvent(self.stopEvent)
    
        def SvcDoRun(self):
            import servicemanager

            servicemanager.LogMsg(servicemanager.EVENTLOG_INFORMATION_TYPE,
                    servicemanager.PYS_SERVICE_STARTED,
                    (self._svc_name_, ''))

            server = Open8055server(9999)
            server.start()

            while True:
                rc = win32event.WaitForSingleObject(self.stopEvent, 5000)

                if rc == win32event.WAIT_OBJECT_0:
                    server.shutdown()
                    servicemanager.LogInfoMsg('open8055server - STOPPED')
                    break
                else:
                    if server.get_status() == 'STOPPED':
                        servicemanager.LogInfoMsg('open8055server - CRASHED')
                        break

    def log(msg):
        servicemanager.LogInfoMsg('open8055server - ' + msg)

    def ctrlHandler(ctrlType):
        return True

    if __name__ == '__main__':
        win32api.SetConsoleCtrlHandler(ctrlHandler, True)
        win32serviceutil.HandleCommandLine(open8055server)
