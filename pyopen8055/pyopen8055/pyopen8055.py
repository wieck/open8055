"""
pyopen8055.py - IO module for the whole 8055 card family.
"""

import ctypes
import math
import re
import sys

def _debug(*args):
    print 'DEBUG:', args
    pass

if sys.platform.startswith('freebsd'):
    import _libusbio as usbio
    import _netio as netio

K8055 = 'K8055'
K8055N = 'K8055N'
OPEN8055 = 'OPEN8055'

TAG_K8055_SET_DEBOUNCE = 1
TAG_K8055_RESET_COUNTER = 3
TAG_K8055_SET_OUTPUT = 5

TAG_K8055N_SET_PROTO = 6
TAG_K8055N_SET_DEBOUNCE = 11
TAG_K8055N_RESET_COUNTER = 13      # 13 and 14
TAG_K8055N_SET_DIGITAL_OUT = 15
TAG_K8055N_GET_DIGITAL_IN = 16
TAG_K8055N_GET_ANALOG_IN = 17
TAG_K8055N_GET_COUNTER = 18        # 18 and 19
TAG_K8055N_GET_COUNTER16 = 20      # 21 and 21
TAG_K8055N_GET_DIGITAL_OUT = 24
TAG_K8055N_GET_ANALOG_OUT = 25

if hasattr(ctypes, 'c_uint8'):
    uint8 = ctypes.c_uint8
    uint16 = ctypes.c_uint16
else:
    uint8 = ctypes.c_ubyte
    uint16 = ctypes.c_ushort

class _k8055_hid_report(ctypes.Structure):
    _pack_ = True
    _fields_ = [
        ('digital_in', uint8),
        ('card_address', uint8),
        ('analog_in_1', uint8),
        ('analog_in_2', uint8),
        ('counter_1_low', uint8),
        ('counter_1_high', uint8),
        ('counter_2_low', uint8),
        ('counter_2_high', uint8),
        ]
class k8055_hid_report(object):
    """
    K8055 HID report packet
    """
    def __init__(self):
        self.cvar = _k8055_hid_report()

    def _get_digital_in(self):
        return self.cvar.digital_in
    digital_in = property(_get_digital_in)

    def _get_analog_in_1(self):
        return self.cvar.analog_in_1
    analog_in_1 = property(_get_analog_in_1)

    def _get_analog_in_2(self):
        return self.cvar.analog_in_2
    analog_in_2 = property(_get_analog_in_2)

    def _get_counter_1_low(self):
        return self.cvar.counter_1_low
    counter_1_low = property(_get_counter_1_low)

    def _get_counter_1_high(self):
        return self.cvar.counter_1_high
    counter_1_high = property(_get_counter_1_high)

    def _get_counter_2_low(self):
        return self.cvar.counter_2_low
    counter_2_low = property(_get_counter_2_low)

    def _get_counter_2_high(self):
        return self.cvar.counter_2_high
    counter_2_high = property(_get_counter_2_high)

    def set_binary_data(self, data):
        ctypes.memmove(ctypes.addressof(self.cvar), data, 8)

##########
# K8055N command packet
##########
class _k8055_hid_command(ctypes.Structure):
    _pack_ = True
    _fields_ = [
        ('command_tag', uint8),
        ('digital_out', uint8),
        ('analog_out_1', uint8),
        ('analog_out_2', uint8),
        ('dummy1', uint8),
        ('dummy2', uint8),
        ('counter_1_debounce', uint8),
        ('counter_2_debounce', uint8),
        ]
class k8055_hid_command(object):
    """
    K8055 command packet
    """
    def __init__(self):
        self.cvar = _k8055_hid_command()

    def _get_command_tag(self):
        return self.cvar.command_tag
    def _set_command_tag(self, val):
        self.cvar.command_tag = val
    command_tag = property(_get_command_tag, _set_command_tag)

    def _get_digital_out(self):
        return self.cvar.digital_out
    def _set_digital_out(self, val):
        self.cvar.digital_out = val
    digital_out = property(_get_digital_out, _set_digital_out)

    def _get_analog_out_1(self):
        return self.cvar.analog_out_1
    def _set_analog_out_1(self, val):
        self.cvar.analog_out_1 = val
    analog_out_1 = property(_get_analog_out_1, _set_analog_out_1)

    def _get_analog_out_2(self):
        return self.cvar.analog_out_2
    def _set_analog_out_2(self, val):
        self.cvar.analog_out_2 = val
    analog_out_2 = property(_get_analog_out_2, _set_analog_out_2)

    def _get_counter_1_debounce(self):
        return self.cvar.counter_1_debounce
    def _set_counter_1_debounce(self, val):
        self.cvar.counter_1_debounce = val
    counter_1_debounce = property(_get_counter_1_debounce, 
            _set_counter_1_debounce)

    def _get_counter_2_debounce(self):
        return self.cvar.counter_2_debounce
    def _set_counter_2_debounce(self, val):
        self.cvar.counter_2_debounce = val
    counter_2_debounce = property(_get_counter_2_debounce, 
            _set_counter_2_debounce)

    def get_binary_data(self):
        return ctypes.string_at(ctypes.addressof(self.cvar), 8)

    def set_binary_data(self, data):
        pass

class pyopen8055:
    """
    Implements a connection to a physically attached K8055(N)/VM110(N)
    Experimental USB Interface Card. 
    """

    ##########
    # pyopen8055 - Creating a new card instance.
    ##########
    def __init__(self, card_address, autosend = True, autorecv = True):
        """
        Open a K8055, K8055N or Open8055 board.
        """
        _debug('pyopen8055.__init__("%s")' % card_address)
        if re.match('^card[0-9+]$', card_address):
            self.io = usbio._usbio(card_address)
        else:
            self.io = netio._netio(card_address)

        self.card_type = self.io.card_type
        if self.card_type == K8055:
            self.recv_buffer = k8055_hid_report()
            self.send_buffer = k8055_hid_command()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

        self.autosend = autosend
        self.autorecv = autorecv

    ##########
    # __del__()
    ##########
    def __del__(self):
        """
        Cleanup on object destruction.
        """
        self.close()

    ##########
    # close()
    ##########
    def close(self):
        """
        Close the connection to this K8055(N) and restore the kernel
        device driver (if there was one on open).
        """
        if self.io is not None:
            self.io.close()
        self.io = None

    ##########
    # set_digital_all()
    ##########
    def set_digital_all(self, value):
        if self.card_type == K8055:
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            self.send_buffer.digital_out = value & 0xff
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_digital_port()
    ##########
    def set_digital_port(self, port, value):
        if self.card_type == K8055:
            if port < 0 or port > 7:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            if bool(value):
                self.send_buffer.digital_out |= (1 << port)
            else:
                self.send_buffer.digital_out &= ~(1 << port)
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_analog_all()
    ##########
    def set_analog_all(self, value1, value2):
        if self.card_type == K8055:
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            self.send_buffer.analog_out_1 = value1
            self.send_buffer.analog_out_2 = value2
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_analog_port()
    ##########
    def set_analog_port(self, port, value):
        if self.card_type == K8055:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_K8055_SET_OUTPUT
            if port == 0:
                self.send_buffer.analog_out_1 = value
            else:
                self.send_buffer.analog_out_2 = value
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # set_counter_debounce_time()
    ##########
    def set_counter_debounce_time(self, port, value):
        pass
        if self.card_type == K8055:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            if value < 1.0 or value > 5000.0:
                raise ValueError('invalid debounce time %f' % value)
            binval = int(round(2.20869 * math.pow(value, 0.5328)))
            if binval < 1:
                binval = 1
            if binval > 255:
                binval = 255
            print 'debounce:', binval

            self.send_buffer.command_tag = TAG_K8055_SET_DEBOUNCE + port
            if port == 0:
                self.send_buffer.counter_1_debounce = binval
            else:
                self.send_buffer.counter_2_debounce = binval
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # reset_counter()
    ##########
    def reset_counter(self, port):
        if self.card_type == K8055:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            self.send_buffer.command_tag = TAG_K8055_RESET_COUNTER + port
            self._autosend()
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_digital_all()
    ##########
    def read_digital_all(self):
        if self.card_type == K8055:
            self._autorecv()
            value = self.recv_buffer.digital_in
            return (((value & 0x10) >> 4) | ((value & 0x20) >> 4) |
                    ((value & 0x01) << 2) | ((value & 0x40) >> 3) |
                    ((value & 0x80) >> 3))
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_counter()
    ##########
    def read_counter(self, port):
        if self.card_type == K8055:
            if port < 0 or port > 1:
                return -1
            self._autorecv()
            if port == 0:
                return (self.recv_buffer.counter_1_low | 
                        (self.recv_buffer.counter_1_high << 8))
            else:
                return (self.recv_buffer.counter_2_low | 
                        (self.recv_buffer.counter_2_high << 8))
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_counter16()
    ##########
    def read_counter16(self, port):
        return self.read_counter(port) & 0xffff

    ##########
    # read_analog_all()
    ##########
    def read_analog_all(self):
        if self.card_type == K8055:
            self._autorecv()
            return self.recv_buffer.analog_in_1, self.recv_buffer.analog_in_2
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # read_analog_port()
    ##########
    def read_analog_port(self, port):
        if self.card_type == K8055:
            if port < 0 or port > 1:
                raise ValueError('invalid port number %d' % port)
            self._autorecv()
            if port == 0:
                return self.recv_buffer.analog_in_1
            else:
                return self.recv_buffer.analog_in_2
        else:
            raise NotImplementedError("Protocol '%s' not implemented" %
                    self.card_type)

    ##########
    # recv()
    ##########
    def recv(self):
        if self.card_type == K8055:
            self.recv_buffer.set_binary_data(self.io.recv_pkt(8))

    ##########
    # send()
    ##########
    def send(self):
        if self.card_type == K8055:
            self.io.send_pkt(self.send_buffer.get_binary_data())
        

    ############################################################
    # Private functions
    ############################################################

    def _autorecv(self):
        if self.autorecv:
            if self.card_type == K8055:
                self.recv_buffer.set_binary_data(self.io.recv_pkt(8))

    def _autosend(self):
        if self.autosend:
            if self.card_type == K8055:
                self.io.send_pkt(self.send_buffer.get_binary_data())

    def _dump_pkt(self, pkt):
        res = []
        for i in range(0, 8):
            res.append('{0:02x}'.format(ord(pkt[i])))
        return ' '.join(res)

