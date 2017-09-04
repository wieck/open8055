
py8055n

A Python_ module to access Velleman K8055(N)/VM110(N) cards based on pylibusb_.

This module requires the Python libusb1 module.

To access the K8055 or Open8055 cards as a regular user (non-root) a group "open8055" should be created and file /etc/udev/rules.d/55-open8055.rules needs to be created with the following content:

SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="5500", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="5501", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="5502", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="5503", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="55f0", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="55f1", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="55f2", ACTION=="add", GROUP="open8055", MODE="0660"
SUBSYSTEMS=="usb", ATTRS{idVendor}=="10cf", ATTRS{idProduct}=="55f3", ACTION=="add", GROUP="open8055", MODE="0660"

The user(s) then need to be added to the open8055 group.

License: BSD

Author: Jan Wieck <JanWieck@Yahoo.com>

.. _libusb1: https://pypi.python.org/pypi/libusb1
.. _py8055n: https://github.com/wieck/py8055n
.. _Python: http://python.org


