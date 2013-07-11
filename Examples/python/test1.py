#!/usr/bin/env python

import sys

import eventloop
import open8055


EFFECT_RUNNING = 1
EFFECT_BAR = 2
EFFECT_KNIGHT = 3
EFFECT_SERVO = 4

current_event = None
current_effect = None
current_state = 0

mainloop = None
card = None
pwm1_state = False

def main(args):
    global card
    global mainloop
    
    cardid = 0
    host = 'localhost'
    port = 8055
    user = 'nobody'
    password = 'nopass'

    card = open8055.open(cardid, host = host, port = port,
            user = user, password = password)

    card.all_defaults()
    card.set_output_all(0x55)

    mainloop = eventloop.EventLoop()
    mainloop.readable(card, card_input)

    mainloop.timer(0.5, pwm1_toggle)

    print 'mainloop running'
    try:
        mainloop.run_mainloop()
    except KeyboardInterrupt:
        print 'KeyboardInterrupt received'
    print 'mainloop returned'

    print 'calling all_defaults()'
    card.all_defaults()
    print 'calling close()'
    card.close()
    print 'main() done'

def pwm1_toggle():
    global pwm1_state

    pwm1_state = not pwm1_state
    if pwm1_state is True:
        card.set_pwm(1, 1.0)
    else:
        card.set_pwm(1, 0.0)

def change_effect(new_effect):
    global current_event
    global current_effect
    global current_state

    if current_event is not None:
        mainloop.cancel(current_event)
        current_event = None

    if new_effect is None:
        return

    current_effect = new_effect
    current_state = 0
    current_event = mainloop.after(0.0, effect_callback)

def effect_callback():
    global current_effect
    global current_state

    if current_effect == EFFECT_RUNNING:
        card.set_output_all(1 << current_state)
        current_state += 1
        if current_state >= 8:
            current_state = 0
        next_call = (0.2 + 0.8 * (1.0 - card.get_adc(0))) / 8.0


    current_event = mainloop.after(next_call, effect_callback)

def card_input():
    global card
    global mainloop
    global current_effect

    try:
        card.poll()
    except Exception as err:
        print 'poll(): ' + str(err)
        mainloop.stop_mainloop()
        return

    card.set_pwm(0, card.get_adc(1))

    if card.get_input(4) is True:
        print 'Button I5 pressed - stopping mainloop'
        mainloop.stop_mainloop()
        return

    if card.get_input(0) is True:
        if current_effect != EFFECT_RUNNING:
            print 'Button I1 pressed - start EFFECT_RUNNING'
            change_effect(EFFECT_RUNNING)
        return
        


if __name__ == '__main__':
    sys.exit(main(sys.argv))
