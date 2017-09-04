import ctypes
import math
import pyopen8055
import usb1
import re

VENDOR_ID = 0x10cf
PID_K8055 = 0x5500
PID_OPEN8055 = 0x55f0

def _debug(*args):
    print 'DEBUG:', args
    pass

class _usbio:
    """
    pylibusb based IO module for pyopen8055.
    """

    ##########
    # _usbio()
    ##########
    def __init__(self, card_address):
        """
        Open a local 8055 type card via pylibusb.
        """
        self.card_address = card_address
        _debug('_libusb_io.__init__("%s")' % card_address)
        m = re.match('^card([0-9]+)$', card_address)
        if not m:
            raise ValueError("invalid card address '%s'" % card_address)
        self.card_number = int(m.groups()[0])
        _debug('  card_number = %d' % self.card_number)

        # ----
        # Open the device
        # ----
        self.cxt = usb1.USBContext()
        found = False
        while True:
            # ----
            # Try opening the specified Open8055 card
            # ----
            self.handle = self.cxt.openByVendorIDAndProductID(
                        VENDOR_ID, PID_OPEN8055 + self.card_number,
                        skip_on_error = True)
            if self.handle is not None:
                self.card_type = pyopen8055.OPEN8055
                found = True
                break

            # ----
            # Try opening the specified K8055(N) card
            # ----
            self.handle = self.cxt.openByVendorIDAndProductID(
                        VENDOR_ID, PID_K8055 + self.card_number,
                        skip_on_error = True)
            if self.handle is not None:
                self.card_type = pyopen8055.K8055
                found = True
                break

            # ----
            # Nothing found
            # ----
            break

        if not found:
            raise RuntimeError("'%s' not found" % card_address)
        _debug("  device open")

        # ----
        # Detach an eventually existing kernel driver on interface 0
        # ----
        if self.handle.kernelDriverActive(0):
            self.had_kernel_driver = True
            self.handle.detachKernelDriver(0)
            _debug("  kernel driver detached")
        else:
            self.had_kernel_driver = False
            _debug("  no kernel driver active")

        # ----
        # Set the active configuration.
        # ----
        #self.handle.setConfiguration(0)
        #_debug("  configuration 0 set")

        # ----
        # Claim interface 0
        # ----
        self.handle.claimInterface(0)
        _debug("  interface claimed")

        # ----
        # If the code above identified the card as a K8055, it could also be
        # a K8055N. Try switching it to K8055N protocol to find out.
        # ----
        if self.card_type == pyopen8055.K8055:
            _debug("  attempting to switch to K8055N protocol")
            buf = ctypes.create_string_buffer(8)
            buf[0] = chr(pyopen8055.TAG_K8055N_SET_PROTO)
            _debug("    sending SET_PROTO")
            self.send_pkt(buf)
            _debug("    reading reply")
            reply = self.recv_pkt(8)
            if ord(reply[1]) > 10:
                _debug("    reply card_id=%d" % ord(reply[1]))
                if ord(reply[1]) <= 20:
                    self.send_pkt(buf)
                while ord(reply[1]) <= 20:
                    reply = self.recv_pkt(8)
                    _debug("    reply card_id=%d" % ord(reply[1]))
                self.card_type = pyopen8055.K8055N
                _debug("  successfully switched to K8055N protocol")
            else:
                _debug("  found original K8055 - no protocol change")

        _debug("  %s: card_type = %s" % (card_address, self.card_type))

    ##########
    # close()
    ##########
    def close(self):
        """
        Close the connection to this card.
        """
        _debug("Closing card")

        # ----
        # Release interface 0
        # ----
        self.handle.releaseInterface(0)
        _debug("  interface released")

        # ----
        # If it had a kernel driver attached, reattach that
        # ----
        if self.had_kernel_driver:
            self.handle.attachKernelDriver(0)
            _debug("  kernel driver attached")

        # ----
        # Close it.
        # ----
        self.handle.close()
        self.handle = None
        _debug("  card closed")

    ##########
    # send_pkt()
    ##########
    def send_pkt(self, buffer):
        rc = self.handle.interruptWrite(0x01, buffer, timeout = 0)
        if rc != len(buffer):
            raise RuntimeError(
                    'interruptWrite returned %d - expected %d' %
                    (rc, len(buffer)))

    ##########
    # recv_pkt()
    ##########
    def recv_pkt(self, length):
        data = str(self.handle.interruptRead(0x81, length, timeout = 0))
        if len(data) != length:
            raise RuntimeError(
                    'interruptRead returned %d - expected %d' %
                    (len(data), length))
        return data

