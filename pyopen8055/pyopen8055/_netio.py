import ctypes
import math
import re

def _debug(*args):
    print 'DEBUG:', args
    pass

class _netio:
    """
    Socket and open8055server based IO module for pyopen8055.
    """

    def __init__(self, card_address):
        self.card_address = card_address
        _debug('_libusb_io.__init__("%s")' % card_address)
        raise NotImplementedError("network IO not implemented yet")
