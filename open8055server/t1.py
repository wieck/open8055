#!/usr/bin/env python

import open8055io
import time

print 'device0 present:', open8055io.present(0)
print 'device1 present:', open8055io.present(1)

open8055io.open(0)
print 'card 0 opened'

open8055io.write(0, '0155');
time.sleep(1.0)
open8055io.write(0, '0100');

print 'packet:', open8055io.read(0)


open8055io.close(0)
print 'card 0 closed'

