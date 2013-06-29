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
    def present(cardNum):
        # ----
        # Try to open the card to see if it is there. Since _open_card()
        # opens it in shared mode, there is no problem if the card is
        # already open. We close it right away and just report if the
        # open succeeded.
        # ----
        cardNum = int(cardNum)
        r, w = _open_card(cardNum)
        if r:
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
    def open(cardNum):
        cardNum = int(cardNum)
        if cardNum in recv_handles:
            raise Exception('card already open')
        r, w = _open_card(cardNum)
        if not r:
            raise Exception('card not present')
        recv_handles[cardNum] = r
        send_handles[cardNum] = w

    # ----------
    # close()
    #
    #   Close a Open8055.
    # ----------
    def close(cardNum):
        cardNum = int(cardNum)
        if cardNum not in recv_handles:
            raise Exception('card not open')

        win32file.CloseHandle(recv_handles[cardNum])
        win32file.CloseHandle(send_handles[cardNum])
        del recv_handles[cardNum]
        del send_handles[cardNum]

    # ----------
    # read()
    #
    #   Read one HID report from an open card.
    # ----------
    def read(cardNum):
        cardNum = int(cardNum)
        if cardNum not in recv_handles:
            raise Exception('card not open')

        hr, ioBuf = win32file.ReadFile(recv_handles[cardNum], 33, None)
        if hr != 0:
            raise IOError('ReadFile() failed')
        if len(ioBuf) != 33:
            raise IOError('short read, expected 33, got ' + str(len(ioBuf)))

        data = ''
        for idx in range(1, 33):
            data += '{0:02X}'.format(ord(ioBuf[idx]))
        print 'len(data):', len(data)
        return data

    # ----------
    # write()
    #
    #   Send one HID command to an open card.
    # ----------
    def write(cardNum, data):
        cardNum = int(cardNum)
        if cardNum not in recv_handles:
            raise Exception('card not open')

        if len(data) > 64:
            raise ValueError('invalid HID packet data')
        ioBuf = chr(0)
        idx = 0
        while idx < len(data):
            ioBuf += chr(int(data[idx:idx+2], 16) & 0xFF)
            idx += 2
        while len(ioBuf) < 33:
            ioBuf += chr(0)
        
        win32file.WriteFile(send_handles[cardNum], ioBuf, None)
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
    def _open_card(cardNum):
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
                OPEN8055_VID, OPEN8055_PID + cardNum)

        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path)
        for i in itertools.count():
            try:
                val = winreg.EnumKey(key, i)
            except:
                break
            key2 = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path +
                                  '\\' + val)
            devinst, type = winreg.QueryValueEx(key2, 'DeviceInstance')
            if not devinst:
                continue

            if devinst.startswith(cardinst):
                key3 = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, path +
                                      '\\' + val + '\\#')
                symlink, type = winreg.QueryValueEx(key3, 'SymbolicLink')
                if not symlink:
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

                return r_handle, w_handle

        return None, None

elif os.name == 'posix':
    from open8055device import *


