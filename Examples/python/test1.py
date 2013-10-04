#!/usr/bin/env python

import os
import sys
import getopt
import getpass

# ----
# Import custom modules that come with Open8055
# ----
import eventloop
import open8055

# ----
# Constants
# ----
EFFECT_RUNNING = 1
EFFECT_BAR = 2
EFFECT_KNIGHT = 3
EFFECT_SERVO = 4

# ----
# Global variables
# ----
current_event = None
current_effect = None
current_state = 0

mainloop = None
card = None
pwm2_state = False

# ----
# main()
# ----
def main(argv):
    global card
    global mainloop
    
    # ----
    # Default values and parsing of command line options
    # ----
    destination = 'card0'
    password = None

    try:
        opts, args = getopt.getopt(argv, 'd:?', 
                ['destination=', 'help'])
    except getopt.GetoptError as err:
        sys.stderr.write('Error: ' + str(err) + '\n')
        usage()
        sys.exit(1)

    for opt, arg in opts:
        if opt in ('-d', '--destination', ):
            destination = arg
        elif opt in ('-?', '--help', ):
            usage()
            return 0

    # ----
    # Finallize username and password
    # ----
    conninfo = open8055.conninfo(destination)
    # if user is None or password is None:
    #     user, password = open8055.username_password(host, port, user, password)
    # if password is None:
    #     password = getpass.getpass('Password for {0}:'.format(user))

    # ----
    # Connect to the server
    # ----
    card = open8055.open(conninfo, password)

    # ----
    # Set the card to default values and then activate the 0x55 pattern on
    # the digital outputs
    # ----
    card.all_defaults()
    card.set_output_all(0x55)

    # ----
    # Start the event loop and add a callback when the server is
    # sending new INPUT data.
    # ----
    mainloop = eventloop.EventLoop()
    mainloop.readable(card, card_input)

    # ----
    # Add a timer callback that we use to toggle PWM2
    # ----
    mainloop.timer(0.5, pwm2_toggle)

    # ----
    # Run the main event loop
    # ----
    print 'mainloop running'
    try:
        mainloop.run_mainloop()
    except KeyboardInterrupt:
        print 'KeyboardInterrupt received'
    print 'mainloop returned'

    # ----
    # Reset the card to default values, close it and exit.
    # ----
    card.all_defaults()
    card.close()
    return 0

# ----
# usage()
# ----
def usage():
    sys.stderr.write("""\nusage: {0} [options]

    Options:
    -c, --card=CARDID       Card number to use (0)
    -h, --host=HOSTNAME     Host of the open8055server (localhost)
    -p, --port=PORT         TCP/IP port number of open8055server (8055)
    -u, --user=USERNAME     Username to authenticate as (nobody)
    \n""".format(os.path.basename(sys.argv[0])))

# ----
# pwm2_toggle()
#
#   Called from the main event loop every 0.5 seconds. We simply toggle
#   the PWM2 output from 0.0 to 1.0, making the LED blink.
# ----
def pwm2_toggle(event):
    global pwm2_state

    pwm2_state = not pwm2_state
    if pwm2_state is True:
        card.set_pwm(1, 1.0)
    else:
        card.set_pwm(1, 0.0)

# ----
# change_effect()
#
#   Switch to a new effect on the digital outputs.
# ----
def change_effect(new_effect):
    global current_event
    global current_effect
    global current_state

    # ----
    # Turn the current effect off (if any) by canceling the event callback.
    # ----
    if current_event is not None:
        mainloop.cancel(current_event)
        current_event = None
    current_effect = None

    # ----
    # Nothing else to do if the new effect is None.
    # ----
    if new_effect is None:
        return

    # ----
    # Start the new effect.
    # ----
    current_effect = new_effect
    current_state = 0
    current_event = mainloop.timer(0.0, effect_callback)

# ----
# effect_callback()
#
#   Called from the event mainloop for timer event based effects.
# ----
def effect_callback(event):
    global current_effect
    global current_state

    if current_effect == EFFECT_RUNNING:
        # ----
        # Running light effect. ADC1 conrols the speed of the loop from
        # 1 to 5 times per second.
        # ----
        card.set_output_all(1 << current_state)
        current_state += 1
        if current_state >= 8:
            current_state = 0
        interval = (0.2 + 0.8 * (1.0 - card.get_adc(0))) / 8.0

    # ----
    # Adjust the speed
    # ----
    event.interval(interval)

# ----
# card_input()
#
#   Called from the event mainloop whenever the server socket is
#   "ready". This may mean new INPUT data or some error.
# ----
def card_input(event):
    global card
    global mainloop
    global current_effect

    # ----
    # Polling the card internally updates all input data, or raises an error.
    # We let the event mainloop exit on error.
    # ----
    try:
        card.poll()
    except Exception as err:
        print 'poll(): ' + str(err)
        mainloop.stop_mainloop()
        return

    # ----
    # PWM1 traces ADC2.
    # ----
    card.set_pwm(0, card.get_adc(1))

    # ----
    # If button I5 is pressed, stop the mainloop.
    # ----
    if card.get_input(4) is True:
        print 'Button I5 pressed - stopping mainloop'
        mainloop.stop_mainloop()
        return

    # ----
    # If button I1 is pressed, switch to effect RUNNING LIGHT
    # ----
    if card.get_input(0) is True:
        if current_effect != EFFECT_RUNNING:
            print 'Button I1 pressed - start EFFECT_RUNNING'
            change_effect(EFFECT_RUNNING)
        return

# ----
# Call main()
# ----
if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))


