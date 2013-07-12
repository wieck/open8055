"""
This module provides direct USB access to an Open8055 USB experiment
board. It uses PyWIN32 on Windows and a C extension module based on
libusb on Unix.
"""

# ----------------------------------------------------------------------
# open8055io.py
#
#	OS agnostic USB device access for Open8055
#
# ----------------------------------------------------------------------
#
#  Copyright (c) 2012, Jan Wieck
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

# ----------
# On Windows platforms we use PyWIN32 to do the IO.
# ----------
if os.name == 'nt':

    import _winreg as winreg
    import itertools
    import win32file

    OPEN8055_GUID = '4d1e55b2-f16f-11cf-88cb-001111000030'
    OPEN8055_VID = 0x10cf
    OPEN8055_PID = 0x55f0

    recv_handles = {}
    send_handles = {}

    # ----------
    # present()
    #
    #   Check if a given card is present in the system.
    # ----------
    def present(card_num):
        # ----
        # Try to open the card to see if it is there. Since _open_card()
        # opens it in shared mode, there is no problem if the card is
        # already open. We close it right away and just report if the
        # open succeeded.
        # ----
        card_num = int(card_num)
        r, w = _open_card(card_num)
        if r is not None:
            ret = 1
        else:
            ret = 0
        win32file.CloseHandle(r)
        win32file.CloseHandle(w)
        return ret

    # ----------
    # open()
    #
    #   Open a Open8055.
    # ----------
    def open(card_num):
        card_num = int(card_num)
        if card_num in recv_handles:
            raise Exception('card already open')
        r, w = _open_card(card_num)
        if r is None:
            raise Exception('card not present')
        recv_handles[card_num] = r
        send_handles[card_num] = w

    # ----------
    # close()
    #
    #   Close a Open8055.
    # ----------
    def close(card_num):
        card_num = int(card_num)
        if card_num not in recv_handles:
            raise Exception('card not open')

        win32file.CloseHandle(recv_handles[card_num])
        win32file.CloseHandle(send_handles[card_num])
        del recv_handles[card_num]
        del send_handles[card_num]

    # ----------
    # read()
    #
    #   Read one HID report from an open card.
    # ----------
    def read(card_num):
        card_num = int(card_num)
        if card_num not in recv_handles:
            raise Exception('card not open')

        hr, iobuf = win32file.ReadFile(recv_handles[card_num], 33, None)
        if hr != 0:
            raise IOError('ReadFile() failed')
        if len(iobuf) != 33:
            raise IOError('short read, expected 33, got ' + str(len(iobuf)))

        return iobuf[1: 33]

    # ----------
    # write()
    #
    #   Send one HID command to an open card.
    # ----------
    def write(card_num, data):
        card_num = int(card_num)
        if card_num not in recv_handles:
            raise Exception('card not open')

        if len(data) > 64:
            raise ValueError('invalid HID packet data')
        iobuf = chr(0) + data
        while len(iobuf) < 33:
            iobuf += chr(0)
        
        win32file.WriteFile(send_handles[card_num], iobuf, None)
        return 32

    # ----------
    # _open_card()
    #
    #   Attempt to open a card. If successful, we return a tuple
    #   of two handles, one for reading and another for writing.
    #   This makes it safe to do input and output simultaneously
    #   in two threads.
    #   On failure we return a tuple of None values.
    # ----------
    def _open_card(card_num):
        # ----
        # Unfortunately PyWIN32 does not expose the SetupDi...
        # interface to search for the current SymbolicLink of a
        # connected USB interface. So we are going to query the
        # registry and try to open all devices that belong to the
        # known GUID and have the requested Vid and Pid.
        # ----
        path = 'SYSTEM\\CurrentControlSet\\Control\\DeviceClasses'
        path = path + '\\{' + OPEN8055_GUID + '}'

        cardinst = 'HID\\Vid_{0:04x}&Pid_{1:04x}\\'.format(
                OPEN8055_VID, OPEN8055_PID + card_num)

        # ----
        # Loop over all registry keys in the DeviceClasses
        # ----
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path)
        for i in itertools.count():
            try:
                val = winreg.EnumKey(key, i)
            except:
                break

            # ----
            # See if that key has a value named DeviceInstance and
            # if that value starts with our cardinst prefix.
            # ----
            key2 = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path +
                                  '\\' + val)
            devinst, type = winreg.QueryValueEx(key2, 'DeviceInstance')
            if not devinst:
                continue

            if devinst.startswith(cardinst):
                # ----
                # Found one. There can be multiple of theses. Windows
                # creates a new entry for every USB port, this device
                # is connected to, and never removes them.
                #
                # Get the SymbolicLink value from the '#' subkey and
                # try to open the device.
                # ----
                key3 = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path +
                                      '\\' + val + '\\#')
                symlink, type = winreg.QueryValueEx(key3, 'SymbolicLink')
                if symlink is None:
                    continue
                try:
                    r_handle = win32file.CreateFile(symlink,
                            win32file.GENERIC_READ,
                            win32file.FILE_SHARE_READ |
                                win32file.FILE_SHARE_WRITE,
                            None, win32file.OPEN_EXISTING, 0, 0)
                except:
                    continue

                try:
                    w_handle = win32file.CreateFile(symlink,
                            win32file.GENERIC_WRITE,
                            win32file.FILE_SHARE_READ |
                                win32file.FILE_SHARE_WRITE,
                            None, win32file.OPEN_EXISTING, 0, 0)
                except:
                    win32file.CloseHandle(r_handle)
                    continue

                # ----
                # Success. Return the two handles.
                # ----
                return r_handle, w_handle

        # ----
        # Card not found.
        # ----
        return None, None

# ----------
# On Posix platforms the above functions are implemented as a
# Python C extension using libusb. 
# ----------
elif os.name == 'posix':
    from open8055device import *


