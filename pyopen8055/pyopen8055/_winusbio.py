import ctypes
import math
import pyopen8055
import re
import uuid

def _debug(*args):
    print 'DEBUG:', args
    pass

BYTE = ctypes.c_byte
WORD = ctypes.c_ushort
DWORD = ctypes.c_ulong
BOOLEAN = BYTE
LPCSTR = ctypes.c_char_p
HANDLE = ctypes.c_void_p

class SP_DEVICE_INTERFACE_DATA(ctypes.Structure):
    _fields_ = [
        ('cbSize', DWORD),
        ('Guid', ctypes.c_char * 16),
        ('DevInst', DWORD),
        ('Reserve', ctypes.POINTER(DWORD))
    ]

setupapi = ctypes.windll.setupapi
kernel32 = ctypes.windll.kernel32

DIGCF_PRESENT = 0x00000002
DIGCF_DEVICEINTERFACE = 0x00000010

GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
FILE_SHARE_READ = 0x00000001
FILE_SHARE_WRITE = 0x00000002
FILE_SHARE_DELETE = 0x00000004
CREATE_NEW = 0x00000001
CREATE_ALWAYS = 0x00000002
OPEN_EXISTING = 0x00000003
OPEN_ALWAYS = 0x00000004
TRUNCATE_EXISTING = 0x00000005

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
        _debug('_winusbio.__init__("%s")' % card_address)
        m = re.match('^card([0-9]+)$', card_address)
        if not m:
            raise ValueError("invalid card address '%s'" % card_address)
        self.card_number = int(m.groups()[0])
        _debug('  card_number = %d' % self.card_number)
        if self.card_number < 0 or self.card_number > 3:
            raise ValueError("invalid card address '%s'" % card_address)

        self.send_handle = None
        self.recv_handle = None

        vid_pid_str = 'hid#vid_%04x&pid_%04x' % (
                pyopen8055.K8055_VID, pyopen8055.K8055_PID + self.card_number)

        # ----
        # Find the path of the requested card. First get the
        # device info set using the class GUID.
        # ----
        k8055_uuid = uuid.UUID('{4d1e55b2-f16f-11cf-88cb-001111000030}')
        c_uuid = ctypes.create_string_buffer(k8055_uuid.bytes_le, 16)
        dev_info_set = setupapi.SetupDiGetClassDevsA(
                ctypes.byref(c_uuid),
                None, 0, 
                DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)
        
        # ----
        # Enumerate that set and check all device paths we find. We are
        # looking for a path that contains that magic "hid#vid_10cf&pid_550x
        # ----
        dev_interface_data = SP_DEVICE_INTERFACE_DATA()
        dev_interface_data.cbSize = ctypes.sizeof(SP_DEVICE_INTERFACE_DATA)
        index = 0
        device_path = None
        while setupapi.SetupDiEnumDeviceInterfaces(dev_info_set, None,
                ctypes.byref(c_uuid), index, 
                ctypes.byref(dev_interface_data)):
            # ----
            # First we have to call SetupDiGetDeviceInterfaceDetailA()
            # with a NULL pointer to get the required size to store the
            # path.
            # ----
            required_size = DWORD()
            setupapi.SetupDiGetDeviceInterfaceDetailA(dev_info_set,
                    ctypes.byref(dev_interface_data), None, 0,
                    ctypes.byref(required_size), None)
            # ----
            # Now we need to (re)create the class for the detail_data
            # structure so that it represents the correct size.
            # ----
            class SP_DEVICE_INTERFACE_DETAIL_DATA(ctypes.Structure):
                _fields_ = [
                    ('cbSize', DWORD),
                    ('DevicePath', ctypes.c_char * required_size.value)
                ]
            # ----
            # And then we are ready to actually receive the path data.
            # Microsoft could not possibly made this any more convoluted.
            # ----
            detail_data = SP_DEVICE_INTERFACE_DETAIL_DATA()
            detail_data.cbSize = 5
            if not setupapi.SetupDiGetDeviceInterfaceDetailA(dev_info_set,
                    ctypes.byref(dev_interface_data),
                    ctypes.byref(detail_data), required_size,
                    ctypes.byref(required_size), None):
                setupapi.SetupDiDestroyDeviceInfoList(dev_info_set)
                raise RuntimeError('SetupDiGetInterfaceDetailA() failed')

            # ----
            # Check if we found our card.
            # ----
            if not str(detail_data.DevicePath).find(vid_pid_str) < 0:
                device_path = str(detail_data.DevicePath)
                break

            index += 1

        # ----
        # Cleanup of resources allocated inside of setupapi.dll
        # ----
        setupapi.SetupDiDestroyDeviceInfoList(dev_info_set)

        if device_path is None:
            raise RuntimeError("K8055 type card '%s' not found" % card_address)

        # ----
        # Now try to open the send and recv handles.
        # ----
        self.recv_handle = kernel32.CreateFileA(
                ctypes.byref(ctypes.create_string_buffer(device_path)),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, None, OPEN_EXISTING, 0, 0)
        self.send_handle = kernel32.CreateFileA(
                ctypes.byref(ctypes.create_string_buffer(device_path)),
                GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE, None, OPEN_EXISTING, 0, 0)

        # ----
        # Set the active configuration. The K8055/K8055N has only one, so
        # the existence of multiple configurations identifies the Open8055.
        # ----
        _debug("  TODO: Windows doesn't know about configurations!!!")
        self.card_type = pyopen8055.K8055

        # ----
        # If the card had only one configuration, it could be either a
        # K8055 or a K8055N. Try switching it to K8055N protocol to find out.
        # ----
        if self.card_type == pyopen8055.K8055:
            _debug("  attempting to switch to K8055N protocol")
            buf = ctypes.create_string_buffer(8)
            buf[0] = chr(pyopen8055.TAG_K8055N_SET_PROTO)
            _debug("    sending SET_PROTO")
            self.send_pkt(buf.raw)
            _debug("    reading reply")
            reply = self.recv_pkt(8)
            if ord(reply[1]) > 10:
                _debug("    reply card_id=%d" % ord(reply[1]))
                if ord(reply[1]) <= 20:
                    self.send_pkt(buf.raw)
                while ord(reply[1]) <= 20:
                    reply = self.recv_pkt(8)
                    _debug("    reply card_id=%d" % ord(reply[1]))
                self.card_type = pyopen8055.K8055N
                _debug("  successfully switched to K8055N protocol")
            else:
                _debug("  found original K8055 - no protocol change")

        _debug("  %s: card_type = %s" % (card_address, self.card_type))

    def __del__(self):
        self.close()

    ##########
    # close()
    ##########
    def close(self):
        """
        Close the connection to this card.
        """
        if self.recv_handle is not None:
            kernel32.CloseHandle(self.recv_handle)
            self.recv_handle = None
        if self.send_handle is not None:
            kernel32.CloseHandle(self.send_handle)
            self.send_handle = None

    ##########
    # send_pkt()
    ##########
    def send_pkt(self, buffer):
        out_packet = ctypes.create_string_buffer('\x00' + buffer,
                len(buffer) + 1)
        bytes_written = DWORD()
        rc = kernel32.WriteFile(self.send_handle, ctypes.byref(out_packet),
                len(out_packet), ctypes.byref(bytes_written), None)
        if rc == 0:
            raise RuntimeError('WriteFile() returned rc %d' % rc)
        if bytes_written.value != len(out_packet):
            raise RuntimeError('WriteFile() wrote %d bytes - expected %d' %
                    bytes_written.value, len(out_packet))

    ##########
    # recv_pkt()
    ##########
    def recv_pkt(self, length):
        in_packet = ctypes.create_string_buffer(length + 1)
        bytes_read = DWORD()
        rc = kernel32.ReadFile(self.recv_handle, ctypes.byref(in_packet),
                length + 1, ctypes.byref(bytes_read), None)
                
        if rc == 0:
            raise RuntimeError('ReadFile() returned rc %d' % rc)
        if bytes_read.value != length + 1:
            raise RuntimeError('ReadFile() read %d bytes - expected %d' %
                    bytes_read.value, length + 1)
        return in_packet[1:]

