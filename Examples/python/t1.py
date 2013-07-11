#!/usr/bin/env python

import sys
import time
import signal

import open8055

signal.signal(signal.SIGINT, signal.SIG_DFL)

print 'available cards:', open8055.cards()

card = open8055.open(0, timeout=5.0)
print 'card 0 open'

print 'get_input_all():', card.get_input_all()
print 'get_input_port(0):', card.get_input(0)
print 'get_counter(0):', card.get_counter(0)
print 'get_adc(0):', card.get_adc(0)

card.set_output_all(0x55)
time.sleep(1.0)
card.set_output_all(0x00)
time.sleep(1.0)

card.autoflush = False
for port in range(0, 8):
    card.set_output(port, True)
    card.flush()
    time.sleep(0.1)
    card.set_output(port, False)
card.autoflush = True

for port in range(0, 8):
    card.set_output_all(1 << port)
    time.sleep(0.1)
card.set_output_all(0)

print 'get_output_all():', card.get_output_all()
print 'get_output_port(0):', card.get_output(0)

card.close()
print 'card 0 closed'

