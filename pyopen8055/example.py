#!/usr/bin/env python
"""
example.py - 
"""

import sys
import pyopen8055
        
def main(argv):
    card = pyopen8055.pyopen8055('card0')
    #try:
    #    card = pyopen8055.pyopen8055('card0')
    #except Exception as err:
    #    sys.stderr.write(str(err) + '\n')
    #    return 1
    print "card0 of type '%s' opened" % card.card_type

    print 'DIGITAL_OUT:', card.readback_digital_all()
    print 'ANALOG_OUT:', card.readback_analog_all()
    print 'IN_1:', card.get_input(0)
    print 'A_1:', card.get_adc(0)

    card.set_output_all(0x54)
    card.set_output(0, True)
    card.set_debounce(0, 1)
    card.set_debounce(1, 1000)
    card.reset_counter(1)
    cur_d = -1
    cur_c1 = -1
    cur_c2 = -1
    cur_a1 = -1
    cur_a2 = -1

    print ''
    print '===================='
    print 'press digital input 5 to end example'
    print '===================='
    print ''
    #card.autosend = False
    #card.autorecv = False
    #card.recv()
    while True:
        #card.recv()
        new_d = card.get_input_all()
        if new_d != cur_d:
            cur_d = new_d
            print 'DIGITAL:', new_d
        new_c1 = card.get_counter(0)
        if new_c1 != cur_c1:
            cur_c1 = new_c1
            print 'COUNTER_1:', new_c1
        new_c2 = card.get_counter(1)
        if new_c2 != cur_c2:
            cur_c2 = new_c2
            print 'COUNTER_2:', new_c2
        new_a1, new_a2 = card.get_adc_all()
        if new_a1 != cur_a1:
            cur_a1 = new_a1
            print 'ANALOG_1:', new_a1
        if new_a2 != cur_a2:
            cur_a2 = new_a2
            print 'ANALOG_2:', new_a2
        card.set_pwm_all(new_a1, new_a2)
        if (new_d & 0x10) != 0:
            break
        #card.send()
    card.set_output_all(0x00)
    card.set_pwm_all(0, 0)
    #card.send()
    card.close()

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

