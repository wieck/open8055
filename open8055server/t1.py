#!/usr/bin/env python

import threading
import open8055io
import time
import signal

signal.signal(signal.SIGINT, signal.SIG_DFL)

class Reader(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

        self.terminate = False

    def run(self):
        while not self.terminate:
            data = open8055io.read(0)
            print 'RECV:', data

print 'device0 present:', open8055io.present(0)
print 'device1 present:', open8055io.present(1)

open8055io.open(0)
print 'card 0 opened'

reader = Reader()
reader.start()

open8055io.write(0, '0155');
time.sleep(1.0)
open8055io.write(0, '0100');

for i in range(0, 250):
    open8055io.write(0, '01FF')
    time.sleep(0.01)
    open8055io.write(0, '0100')
    time.sleep(0.01)
    open8055io.write(0, '0155')
    time.sleep(0.01)
    open8055io.write(0, '0100')
    time.sleep(0.01)

reader.terminate = True
reader.join()

open8055io.close(0)
print 'card 0 closed'

