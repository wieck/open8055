--------------------------------------------------------------------------------

Open8055 server

    The open8055server is a Python script that serves Open8055 USB Experiment
    boards over the network. Client applications use TCP/IP sockets to access
    the Open8055 boards through this server.

    The server runs as a daemon process on Unix and as a Service on Windows.

    This has several advantages.

    1)  The Open8055 boards can be accessed remotely over the network.

    2)  The access to the board is abstracted into a TCP/IP socket. Any
    	programming language, that is capable of communicating over TCP/IP
	sockets with a clear text line protocol (telnet protocol) can access
	Open8055 boards.

    3)	An application accessing an HID type USB device directly can leave the
    	HID driver stack in a screwed up state, if it crashes or terminates
	without cleanly closing the device. Since the client application is not
	accessing the HID device directly, this cannot happen. The server will
	always properly close the device.

--------------------------------------------------------------------------------

Configuration:

    The file open8055.conf controls which user from which host can access which
    cards using a specific authentication method. The file is written in Python
    ConfigParser syntax.

    The file open8055.users contains the user names, passwords (md5 hashed) and
    a superuser flag for future use.

    The individual lines in the config file consist of 3 items. The network
    address and netmask given as ADDR/WIDTH, the username (or keyword "any")
    and the access type, one of "trust", "plain", "md5" or "deny".

    For example, the line

    	192.168.0.0/16 any md5

    specifies that any user, connecting from an address in 192.168.0.0/16 (this
    is the IPV4 Class C Private Address Space as per RFC 1918) is allowed to
    connect to the server and authenticate with double MD5 encrypted passwords
    as explained below under "Network security".

    The lines are parsed in order and parsing stops at the first address/mask
    and username match. 

--------------------------------------------------------------------------------

Network security:

    Open8055server assumes a trusted network. The client/server protocol is NOT
    encrypted. An attacker could use IP spoofing to inject commands into an
    established connection. By default, the server uses "trust" for all clients
    from "localhost" and "md5" for all clients from Private Networks. If someone
    can spoof packets from localhost, you have a much more serious problem than
    having someone controlling your Open8055 board without permission. Your
    entire computer got rooted and you are no longer in control. 

    That said, the client authentication does support double hashed MD5
    password exchange, so passwords never need to be exchanged in clear text
    over the network or even be stored in clear text (in the .open8055rc or
    open8055.users files).

    On the server side, passwords are usually stored as MD5 hashes in the file
    open8055.users. When a client connects, the server sends a random string
    to the client. The client will concatenate the MD5 of the password (either
    from the .open8055rc or by hashing the actual clear text password) with
    that random string (called a "salt"). The resulting string is again hashed
    with the MD5 algorithm. The server then compares this MD5 hash to what
    results when it hashes the same combination of the password MD5, found in
    the server's open8055.users and the random salt. Since the "salt" is a
    random string, this prevents "sniff and repeat" attacks.

--------------------------------------------------------------------------------

Installing the open8055server Service on Windows:

    In a Command Line window change directory into the ...\open8055server
    directory and type

        open8055service install

    The open8055service.bat script accepts the commands install, remove, start,
    stop and restart. 

--------------------------------------------------------------------------------

Installing the open8055server Service on Unix:

    TODO: Create a service start/stop script for Unix

------------------------------------------------------------------------------
