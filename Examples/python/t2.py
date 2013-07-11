#!/usr/bin/env python

import time
import eventloop
import sys

myloop = eventloop.EventLoop()

def cb1(what):
    print 'cb1 called for', what, 'at', time.asctime()
    if what == 'after':
        myloop.after(6.0, cb1, 'after')

def cb2():
    line = sys.stdin.readline().strip()
    if line == 'forever':
        myloop.forever()
    elif line == 'error':
        raise Exception('I was told to raise an error')
    print 'cb2 received "{0}"'.format(line)


myloop.after(30.0, myloop.stop_mainloop)
myloop.timer(3.0, cb1, 'timer')
myloop.after(6.0, cb1, 'after')
myloop.readable(sys.stdin, cb2)

try:
    myloop.run_mainloop()
except KeyboardInterrupt:
    pass
