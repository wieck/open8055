"""
This module provides access to Open8055 USB experiment cards via TCP/IP
connection to an Open8055Server instance.

Functions:

cards() -- return a list of cards that are available on the server
open() -- connect to the server and open the specified card
"""

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

import os
import socket
import sys

# ----------
# Port configuration mode values
# ----------
MODE_ADC10      = 10
MODE_ADC9       = 11
MODE_ADC8       = 12

MODE_INPUT      = 20
MODE_FREQUENCY  = 21

MODE_OUTPUT     = 30
MODE_SERVO      = 31
MODE_ISERVO     = 32

MODE_PWM        = 40

# ----------
# HID command and report types encoded in the first byte
# ----------
OUTPUT          = 0x01
GETINPUT        = 0x02
SETCONFIG1      = 0x03
GETCONFIG       = 0x04
SAVECONFIG      = 0x05
SAVEALL         = 0x06
RESET           = 0x7F
INPUT           = 0x81


# ----
# cards()
# ----
def cards(host = 'localhost', port = 8055, user = 'nobody', 
        password = 'nopass', timeout = None):
    """
    Returns the available cards on the server as an integer sequence.
    """
    # ----
    # Connect to the server.
    # ----
    card = Open8055()
    card._connect(host, port, user, password, timeout)

    # ----
    # Send the LIST command and read the reply.
    # TODO: need to do password MD5 encryption here.
    # ----
    card._send_message('LIST {0} {1}'.format(user, password))
    line = card._recv_message(timeout)
    msg = line.split(' ')
    if msg[0] != 'LIST':
        raise Exception(
                'expected LIST message, got "{0}" instead'.format(line))
    result = [int(c) for c in msg[1:]]

    # ----
    # Disconnect from the server and return the lists of cards.
    # ----
    error = None
    try:
        card.send_socket.shutdown(socket.SHUT_RDWR)
        card.send_socket.close()
        card.recv_socket.close()
    except Exception as exc:
        error = exc

    card.send_socket = None
    card.recv_socket = None

    if error is not None:
        raise error

    return result


# ----
# open()
# ----
def open(cardid, host = 'localhost', port = 8055, user = 'nobody', 
        password = 'nopass', timeout = None):
    """
    Open an Open8055 card on the server.

    This will also receive the first three messages from the card,
    which are always the CONFIG1, OUTPUT and INPUT reports. The
    timeout argument is applied to establishing the TCP/IP
    connection and each single IO operation, not the whole process.
    """
    # ----
    # Connect to the server.
    # ----
    card = Open8055()
    card._connect(host, port, user, password, timeout)

    # ----
    # Send the OPEN command.
    # TODO: need to do password MD5 encryption here.
    # ----
    card._send_message('OPEN {0} {1} {2}'.format(cardid, user, password))
    
    # ----
    # After connecting and opening a card, the server makes sure
    # that we receive a CONFIG1 and OUTPUT message as well as a
    # forced INPUT report. We can simply wait for that INPUT and
    # let poll() do all the work.
    # ----
    while card.cur_input is None:
        card.poll(timeout)

    return card


# ----------
# Open8055
# ----------
class Open8055:
    """
    Object representing one Open8055 USB experiment board.

    General methods:

    close() -- close the connection to the server
    flush() -- send output and configuration changes to the server
    poll() -- check server for new input states and process all of them
    poll_single() -- check server for new input and process only one
    fileno() -- return the receive file desctiptor for use in select()
    all_defaults() -- set all configuration and outputs to defaults
    reset() -- send a reset signal to the card causing the PIC to reboot

    Methods related to digital inputs:

    set_input_mode() -- set the operating mode of a digital input
    get_input_mode() -- get the current operating mode of a digital input
    get_input_all() -- get the state of all digital input ports
    get_input() -- get the state of one digital input port
    get_counter() -- get the counter/frequency of a digital input port
    reset_counter() -- reset the counter of a digital input port
    set_debounce() -- set the debounce time of a digital input port
    get_debounce() -- get the current debounce time of a digital input port

    Methods related to analog inputs:

    set_adc_mode() -- set the operating mode of an analog input
    get_adc_mode() -- get the current operationg mode of an analog input
    get_adc() -- get the current value of an analog input

    Methods related to digital outputs:

    set_output_mode() -- set the operating mode of a digital output
    get_output_mode() -- get the current operating mode of a digital output
    set_output_all() -- set the state of all digital output ports
    get_output_all() -- get the current state of all digital output ports
    set_output() -- set the state of one digital output port
    get_output() -- get the state of one digital output port
    set_output_servo() -- set the pulse width of a port in servo mode
    get_output_servo() -- get the current pulse width of a port in servo mode

    Methods related to PWM outputs:

    set_pwm_mode() -- set the operating mode of a PWM output
    get_pwm_mode() -- get the current operating mode of a PWM output
    set_pwm() -- set the duty cycle of a PWM output
    get_pwm() -- get the current duty cycle of a PWM output

    Variables:

    autoflush -- if to send changes immediately or via flush()
    """
    def __init__(self):
        self.cardid = None              # Card number on the server
        self.cur_input = None           # Last HID report of type INPUT
        self.cur_output = None          # Current OUTPUT data
        self.cur_config1 = None         # Current CONFIG1 data

        self.autoflush = True           # If to send changes immediately
        self.pend_output = False        # Pending OUTPUT to flush
        self.pend_config1 = False       # Pending CONFIG1 to flush

        self.input_buffer = ''          # Input buffer used in poll()
        self.socket = None              # Connection to the server

    def __del__(self):
        try:
            self.close()
        except:
            pass

    def close(self, timeout = None):
        """
        Close the connection to the card. 
        """
        # ----
        # Check that this card is actually still open.
        # ----
        if self.socket is None:
            return

        # ----
        # Send out all remaining changes to output or configuration.
        # ----
        self.flush()

        # ----
        # Send the QUIT command to the server and wait until it
        # closes the connection. If we don't do it this way and
        # simply close the connection, the server will likely 
        # see a "Connection reset by peer" error. We don't want
        # to have our server logs full of those.
        # ----
        error = None
        if self.socket is not None:
            try:
                self._send_message('QUIT')
                self.socket.settimeout(timeout)
                while self.socket.recv(4096) != '':
                    pass
                self.socket.shutdown(socket.SHUT_RDWR)
                self.socket.close()
            except Exception as exc:
                error = exc
            finally:
                self.socket = None
        if error is not None:
            raise error

    def poll(self, timeout = 0.0):
        """
        Attempts to receive data from the server and process all available
        messages, updating the internal status data accordingly.

        If used in connection with select(), it should be called with
        a 0.0 timeout (default) to process all available messages and
        update the known state of the card.

        Returns True if any message(s) were received, False otherwise.
        """
        # ----
        # See if there is at least one line available.
        # ----
        line = self._recv_message(timeout)
        if line is None:
            return False

        # ----
        # Process that.
        # ----
        self._poll_process_msg(line)

        # ----
        # Now process any other complete message we received so far.
        # ----
        while True:
            idx = self.input_buffer.find('\n')
            if idx < 0:
                return True
            
            line = self.input_buffer[0:idx]
            self.input_buffer = self.input_buffer[idx + 1:]
            self._poll_process_msg(line)

    def poll_single(self, timeout = 0.0):
        """
        Attempts to receive data from the server and process one message.
        The implementation uses an internal receive buffer and eventually
        receives multiple messages with one underlying socket.recv().

        If used in connection with select(), poll_single() should be called
        in a loop with the default 0.0 second timeout until it returns 
        False or an exception occurs. Otherwise the next call to select()
        will wait until the server sends more messages, although messages
        are readily available in the internal receive buffer.

        If you are not interested in single state changes to the input
        ports, use poll() instead.

        Returns True if a message was received, False otherwise.
        """
        # ----
        # See if there is at least one line available.
        # ----
        line = self._recv_message(timeout)
        if line is None:
            return False

        # ----
        # Process that.
        # ----
        self._poll_process_msg(line)
        return True

    def _poll_process_msg(self, line):
        """
        Internal function to interpret one server message.
        """
        msg = line.split(' ')
        if msg[0] == 'RECV':
            # ----
            # Message is of type RECV, which is the content of one
            # HID report.
            # ----
            hid_type = int(msg[1])
            values = [int(x) for x in msg[2:]]
            if hid_type == INPUT:
                self.cur_input = dict((
                            ('msg_type', hid_type),
                            ('input_bits', values[0]),
                            ('input_counter', values[1:6]),
                            ('input_adc_value', values[6:8]),
                        ))
            elif hid_type == OUTPUT:
                self.cur_output = dict((
                            ('msg_type', hid_type),
                            ('output_bits', values[0]),
                            ('output_value', values[1:9]),
                            ('output_pwm_value', values[9:11]),
                            ('reset_counter', values[11]),
                        ))
            elif hid_type == SETCONFIG1:
                self.cur_config1 = dict((
                            ('msg_type', hid_type),
                            ('mode_adc', values[0:2]),
                            ('mode_input', values[2:7]),
                            ('mode_output', values[7:15]),
                            ('mode_pwm', values[15:17]),
                            ('debounce_value', values[17:22]),
                            ('card_address', values[22]),
                        ))
            else:
                raise Exception(
                        'received unknown HID type 0x{0:02X}'.format(hid_type))
        
        elif msg[0] == 'ERROR':
            # ----
            # Report the server ERROR without the message type.
            # ----
            raise Exception('Server error - ' + ' '.join(msg[1:]))
        
        else:
            # ----
            # The server should not send anything else.
            # ----
            raise Exception('Unexpected server message - ' + ' '.join(msg))

    def flush(self):
        """
        Send any outstanding changes, made to the output state or the
        port configurations, to the server.

        This call does nothing in autoflush mode.
        """
        if self.pend_config1:
            self._send_config1()
            self.pend_config1 = False
        if self.pend_output:
            self._send_output()
            self.pend_output = False

    def all_defaults(self):
        """
        Configure the card with all default values as they would be after
        flashing the firmware and booting it for the first time.
        
        Modes and values:

        Digital inputs  = MODE_INPUT, counters reset
        Analog inputs   = MODE_ADC10
        digital outputs = MODE_OUTPUT, state off, servo pulse width 1.5 ms
        PWM outputs     = MODE_PWM, duty 0.0
        """
        self.cur_output = {
            'msg_type': OUTPUT,
            'output_bits': 0,
            'output_value': [18000, 18000, 18000, 18000, 18000, 18000, 
                    18000, 18000],
            'output_pwm_value': [0, 0],
            'reset_counter': 0x1F
        }
        self.cur_config1 = {
            'msg_type': SETCONFIG1,
            'mode_adc': [MODE_ADC10, MODE_ADC10],
            'mode_input': [MODE_INPUT, MODE_INPUT, MODE_INPUT, MODE_INPUT, 
                    MODE_INPUT],
            'mode_output': [MODE_OUTPUT, MODE_OUTPUT, MODE_OUTPUT, MODE_OUTPUT,
                    MODE_OUTPUT, MODE_OUTPUT, MODE_OUTPUT, MODE_OUTPUT],
            'mode_pwm': [MODE_PWM, MODE_PWM],
            'debounce_value': [11, 11, 11, 11, 11],
            'card_address': 0
        }
        self.pend_output = True
        self.pend_config1 = True
        self.flush()

    def reset(self):
        """
        Instruct the micro controller on the Open8055 to reboot.

        As a side effect, the card will disconnect and reconnect from
        the USB, which causes an error and we will lose the connection
        to the server.
        """
        error = None
        if self.socket is not None:
            try:
                self._send_message('SEND ' + str(RESET))
                self.socket.settimeout(None)
                while self.socket.recv(4096) != '':
                    pass
                self.socket.shutdown(socket.SHUT_RDWR)
                self.socket.close()
            except Exception as exc:
                error = exc
            finally:
                self.socket = None
        if error is not None:
            raise error


    def fileno(self):
        """
        Returns the small integer system file number for the receiving
        socket to be used in operations like select() for non-blocking
        reads from the card.
        """
        return self.socket.fileno()
        
    # ----------
    # Functions related to digital input ports
    # ----------

    def set_input_mode(self, port, mode):
        """
        Set the operating mode of a digital input port.

        Modes:

        open8055.MODE_INPUT
            Normal input operation with on/off state and counter.

        open8055.MODE_FREQUENCY 
            The port does not report on states (always off) and the counter
            reflects the number of input signals counted per second. The
            current firmware can reliably measure frequencies up to 2.5 kHz.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        if mode not in (MODE_INPUT, MODE_FREQUENCY,):
            raise ValueError('invalid mode for digital input')
        self.cur_config1['mode_input'][port] = mode

        if self.autoflush:
            self._send_config1()
        else:
            self.pend_config1 = True

    def get_input_mode(self, port):
        """
        Get the current operating mode of a digital input port.

        Modes:

        open8055.MODE_INPUT
            Normal input operation with on/off state and counter.

        open8055.MODE_FREQUENCY 
            The port does not report on states (always off) and the counter
            reflects the number of input signals counted per second. The
            current firmware can reliably measure frequencies up to 2.5 kHz.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        return self.cur_config1['mode_input'][port]

    def get_input_all(self):
        """
        Returns the states of the 5 digital inputs as an integer. 
        """
        return self.cur_input['input_bits']

    def get_input(self, port):
        """
        Returns the state of one digital input as a boolean.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        return (self.cur_input['input_bits'] & (1 << port)) != 0

    def get_counter(self, port):
        """
        Returns the current counter value or frequency of a digital input.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        return self.cur_input['input_counter'][port]

    def reset_counter(self, port):
        """
        Reset the counter of a digital input port.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        self.cur_output['reset_counter'] |= (1 << port)

        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True

    def set_debounce(self, port, seconds):
        """
        Set the debounce timeout of a digital input counter. 

        The debounce time defines how long the input port must be either open
        or closed for being considered "changed" with respect to counting.
        This is used to eliminate multiple counts when mechanical contacts
        open or close, which often actually do "bounce".

        The value is in seconds between 0.0 and 5.0.  The internal precision
        of the firmware is 0.1 milliseconds or 0.0001 seconds.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        if seconds < 0.0 or seconds > 5.0:
            raise ValueError('debounce time out of range')

        self.cur_config1['debounce_value'][port] = int(seconds * 10000) + 1

        if self.autoflush:
            self._send_config1()
        else:
            self.pend_config1 = True

    def get_debounce(self, port):
        """
        Returns the current debounce time of a digital input counter
        in seconds.
        """
        if port < 0 or port > 4:
            raise ValueError('digital input port number out of range')
        return (self.cur_config1['debounce_value'][port] - 1) / 10000.0

    # ----------
    # Functions related to analog inputs
    # ----------

    def set_adc_mode(self, port, mode):
        """
        Set the operating mode of an analog input port.

        K8055 and K8055N boards do not provide a stable, clean reference
        voltage for the ADC. Depending on the power source, the quality
        of the board assembly and electromagnetic noise in the environment
        the can show significant fluctuation in the reported ADC value.

        The lower precision modes allow to sacrifice precision in order
        to reduce the number of input reports sent by the card.

        Modes:

        open8055.MODE_ADC10
            Operate with 10 bits ADC precision.

        open8055.MODE_ADC9
            Operate with 9 bits ADC precision.

        open8055.MODE_ADC8
            Operate with 8 bits ADC precision.
        """
        if port < 0 or port > 1:
            raise ValueError('adc input port number out of range')
        if mode not in (MODE_ADC10, MODE_ADC9, MODE_ADC8,):
            raise ValueError('invalid mode for adc input')
        self.cur_config1['mode_adc'][port] = mode

        if self.autoflush:
            self._send_config1()
        else:
            self.pend_config1 = True

    def get_adc_mode(self, port, mode):
        """
        Get the current operating mode of an analog input port.

        K8055 and K8055N boards do not provide a stable, clean reference
        voltage for the ADC. Depending on the power source, the quality
        of the board assembly and electromagnetic noise in the environment
        the can show significant fluctuation in the reported ADC value.

        The lower precision modes allow to sacrifice precision in order
        to reduce the number of input reports sent by the card.

        Modes:

        open8055.MODE_ADC10
            Operate with 10 bits ADC precision.

        open8055.MODE_ADC9
            Operate with 9 bits ADC precision.

        open8055.MODE_ADC8
            Operate with 8 bits ADC precision.
        """
        if port < 0 or port > 1:
            raise ValueError('adc input port number out of range')
        return self.cur_config1['mode_adc'][port]

    def get_adc(self, port):
        """
        Returns the current value of an analog input. The result is a
        floating point number between 0.0 and 1.0 representing the range
        of the ADC converter. By default that is 0.0V to 5.0V, but it
        can be changed on the card with the attenuator or by installing
        the resistors for sensitivity gain.
        """
        if port < 0 or port > 1:
            raise ValueError('adc port number out of range')

        if self.cur_config1['mode_adc'][port] == MODE_ADC10:
            max_value = 1023.0
        elif self.cur_config1['mode_adc'][port] == MODE_ADC9:
            max_value = 1022.0
        elif self.cur_config1['mode_adc'][port] == MODE_ADC8:
            max_value = 1020.0

        return self.cur_input['input_adc_value'][port] / max_value

    # ----------
    # Functions related to digital output ports.
    # ----------

    def set_output_mode(self, port, mode):
        """
        Set the operating mode of a digital output port.

        Modes:

        open8055.MODE_OUTPUT
            Operate as an on/off switch.

        open8055.MODE_SERVO
            In this mode the output port will generate a pulse width modulated
            signal suitable for controlling standard hobby servos. It consists
            of 40 pulses per second with a pulse width between 0.5 and 2.5
            milliseconds.

        open8055.MODE_ISERVO
            Like MODE_SERVO but with inverted high/low phases. This mode is
            useful for connecting the servo signal line directly to the
            PIC instead of driving it with a pull-up resistor and the
            final output port connector.
        """
        if port < 0 or port > 7:
            raise ValueError('output port number out of range')
        if mode not in (MODE_OUTPUT, MODE_SERVO, MODE_ISERVO,):
            raise ValueError('invalid mode for output port')
        self.cur_config1['mode_output'][port] = mode

        if self.autoflush:
            self._send_config1()
        else:
            self.pend_config1 = True

    def get_output_mode(self, port, mode):
        """
        Get the current operating mode of a digital output port.

        Modes:

        open8055.MODE_OUTPUT
            Operate as an on/off switch.

        open8055.MODE_SERVO
            In this mode the output port will generate a pulse width modulated
            signal suitable for controlling standard hobby servos. It consists
            of 40 pulses per second with a pulse width between 0.5 and 2.5
            milliseconds.

        open8055.MODE_ISERVO
            Like MODE_SERVO but with inverted high/low phases. This mode is
            useful for connecting the servo signal line directly to the
            PIC instead of driving it with a pull-up resistor and the
            final output port connector.
        """
        if port < 0 or port > 7:
            raise ValueError('output port number out of range')
        return self.cur_config1['mode_output'][port]

    def set_output_all(self, value):
        """
        Change the states of all digital outputs, that are configured in
        MODE_OUTPUT. The on/off states are encoded as bits in an 8-bit
        integer value. The bit for ports, that are configured in other
        modes, like MODE_SERVO, are ignored.
        """
        value = int(value) & 0xFF

        self.cur_output['output_bits'] = value

        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True

    def get_output_all(self):
        """
        Retrieve the states of all digital outputs. The on/off states are
        encoded as bits in an 8-bit integer value.
        """
        return self.cur_output['output_bits']

    def set_output(self, port, value):
        """
        Turn one digital output on or off.
        """
        if port < 0 or port > 7:
            raise ValueError('digital output port number out of range')
        if value:
            self.cur_output['output_bits'] |= (1 << port)
        else:
            self.cur_output['output_bits'] &= ~(1 << port)

        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True

    def get_output(self, port):
        """
        Retrieve the current on/off state of one digital output port.
        """
        if port < 0 or port > 7:
            raise ValueError('digital output port number out of range')
        return (self.cur_output['output_bits'] & (1 << port)) != 0

    def set_output_servo(self, port, millisec):
        """
        Set the pulse width of a digital output port for operating modes
        MODE_SERVO and MODE_ISERVO. The value is floating point in
        milliseconds.


        WARNING: The Open8055 Firmware accepts values for the pulse width
        from 0.5 to 2.5 milliseconds. This is a greater range than many
        hobby servos tolerate. Using values outside 1.0 to 2.0 milliseconds
        can damage the servo. Use them at your own risk!
        """
        if port < 0 or port > 7:
            raise ValueError('digital output port number out of range')
        if millisec < 0.5 or millisec > 2.5:
            raise ValueError('pulse width out of range for servo operation')

        # ----
        # The timer used by the firmware to turn the output port off
        # increments 12,000,000 times per second (internal clock frequency
        # of the micro controller). The pulse width is encoded in clock
        # ticks. One millisecond is 12,000 clock ticks.
        # ----
        self.cur_output['output_value'][port] = int(millisec * 12000)

        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True

    def get_output_servo(self, port):
        """
        Returns the current setting for the pulse width of a digital
        output in MODE_SERVO or MODE_ISERVO.
        """
        if port < 0 or port > 7:
            raise ValueError('digital output port number out of range')
        return self.cur_output['output_value'][port] / 12000.0

    def set_pwm_mode(self, port, mode):
        """
        Set the operating mode of a PWM output port

        Modes:

        open8055.MODE_PWM
            Operate as PWM with a frequency of 23.43 kHz

        """
        if port < 0 or port > 1:
            raise ValueError('pwm port number out of range')
        if mode not in (MODE_PWM,):
            raise ValueError('invalid mode for pwm port')
        self.cur_config1['mode_pwm'][port] = mode

        if self.autoflush:
            self._send_config1()
        else:
            self.pend_config1 = True

    def get_pwm_mode(self, port, mode):
        """
        Get the current operating mode of a PWM output port

        Modes:

        open8055.MODE_PWM
            Operate as PWM with a frequency of 23.43 kHz

        """
        if port < 0 or port > 1:
            raise ValueError('pwm port number out of range')
        return self.cur_config1['mode_pwm'][port]

    def set_pwm(self, port, duty):
        """
        Set the duty cycle of a PWM/DAC output port. The duty cycle is
        expressed as a floating point value between 0.0 and 1.0.
        """
        if port < 0 or port > 1:
            raise ValueError('pwm output port number out of range')
        if duty < 0.0 or duty > 1.0:
            raise ValueError('pwm duty cycle out of range')

        self.cur_output['output_pwm_value'][port] = int(duty * 1023)

        if self.autoflush:
            self._send_output()
        else:
            self.pend_output = True

    def get_pwm(self, port):
        """
        Returns the current duty cycle of a PWM/DAC output port. The duty
        cycle is expressed as a floating point value between 0.0 and 1.0.
        """
        if port < 0 or port > 1:
            raise ValueError('pwm output port number out of range')
        return self.cur_output['output_pwm_value'][port] / 1023.0

    def _send_output(self):
        """
        Internal function to send the OUTPUT data to the server.
        """
        msg = 'SEND ' + ' '.join(str(x) for x in
                    [self.cur_output['msg_type']] +
                    [self.cur_output['output_bits']] +
                    self.cur_output['output_value'] +
                    self.cur_output['output_pwm_value'] +
                    [self.cur_output['reset_counter']]
                )
        self._send_message(msg)

    def _send_config1(self):
        """
        Internal function to send the SETCONFIG1 data to the server.
        """
        msg = 'SEND ' + ' '.join(str(x) for x in
                    [self.cur_config1['msg_type']] +
                    self.cur_config1['mode_adc'] +
                    self.cur_config1['mode_input'] +
                    self.cur_config1['mode_output'] +
                    self.cur_config1['mode_pwm'] +
                    self.cur_config1['debounce_value'] +
                    [self.cur_config1['card_address']]
                )
        self._send_message(msg)

    def _connect(self, host, port, user, password, timeout):
        """
        Internal function to connect to the server and process the
        initial HELLO and SALT messages.
        """
        # ----
        # Establish the connetion.
        # ----
        self.socket = socket.create_connection((host, port), timeout)

        # ----
        # The first message from the server should be
        # HELLO Open8055Server <server_version>
        # ----
        line = self._recv_message(timeout)
        msg = line.split(' ')
        if msg[0] != 'HELLO' or msg[1] != 'Open8055Server':
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
                self.socket.close()
            except:
                pass
            raise Exception(
                    'expected HELLO message, got "{0}" instead'.format(line))
        self.server_version = msg[2]

        # ----
        # The second message from the server should be
        # SALT <16_hex_characters>
        # This random data from the server is used to guard against
        # password replay attacks.
        # ----
        line = self._recv_message(timeout)
        msg = line.split(' ')
        if msg[0] != 'SALT':
            try:
                self.socket.shutdown(socket.SHUT_RDWR)
                self.socket.close()
            except:
                pass
            raise Exception(
                    'expected SALT message, got "{0}" instead'.format(line))
        self.server_salt = msg[1]

    def _send_message(self, message):
        """
        Internal function to send a message on to the server.
        """
        self.socket.sendall(message + '\n')

    def _recv_message(self, timeout):
        """
        Internal function to receive data from the server and split off
        the first message, if available.
        """
        idx = self.input_buffer.find('\n')
        if idx < 0:
            self.socket.settimeout(timeout)
            try:
                data = self.socket.recv(4096)
            except socket.timeout:
                return None

            if data == '':
                raise Exception('server closed connection')
            
            self.input_buffer += data
            idx = self.input_buffer.find('\n')
            if idx < 0:
                return None
                
        result = self.input_buffer[0:idx]
        self.input_buffer = self.input_buffer[idx + 1:]

        return result


