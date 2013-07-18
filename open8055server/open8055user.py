#!/usr/bin/env python

import ConfigParser
import getopt
import getpass
import hashlib
import os
import stat
import sys


def main(argv, config_locations):
    flag_admin = None
    flag_mode = None
    users_file = None

    try:
        opts, args = getopt.getopt(argv, '?Aac:df:hNu', [
                'admin', 'add', 'config=', 'delete', 'usersfile=',
                'help', 'noadmin', 'update'])
    except Exception as err:
        sys.stderr.write('Error: ' + str(err))
        usage()
        return 1

    if len(args) != 1:
        usage()
        return 1
    username = args[0]
    try:
        if username.index(':') >= 0:
            sys.stderr.write('Error: Invalid username\n')
            return 2
    except:
        pass

    for opt, arg in opts:
        if opt in ('-h', '-?', '--help', ):
            usage()
            return 0
        elif opt in ('-A', '--admin', ):
            if flag_admin is not None:
                sys.stderr.write('Flags --admin and --noadmin are exclusive\n')
                return 1
            flag_admin = True
        elif opt in ('-N', '--noadmin', ):
            if flag_admin is not None:
                sys.stderr.write('Flags --admin and --noadmin are exclusive\n')
                return 1
            flag_admin = False
        elif opt in ('-c', '--config', ):
            if not os.path.exists(arg):
                sys.stderr.write(arg + ': no such file\n')
                return 1
            config_locations = (arg, )
        elif opt in ('-f', '--usersfile', ):
            if not os.path.exists(arg):
                sys.stderr.write(arg + ': no such file\n')
                return 1
            users_file = arg
        elif opt in ('-a', '--add', ):
            if flag_mode is not None:
                sys.stderr.write(
                        'Flags --{0} and --add are exclusive\n'.format(
                        flag_mode))
                return 1
            flag_mode = 'add'
        elif opt in ('-d', '--delete', ):
            if flag_mode is not None:
                sys.stderr.write(
                        'Flags --{0} and --delete are exclusive\n'.format(
                        flag_mode))
                return 1
            flag_mode = 'delete'
        elif opt in ('-u', '--update', ):
            if flag_mode is not None:
                sys.stderr.write(
                        'Flags --{0} and --update are exclusive\n'.format(
                        flag_mode))
                return 1
            flag_mode = 'update'

    # ----
    # Locate the users file. If not explicitly given by the --usersfile
    # option, the path is specified in the config file. It can be relative
    # to the config file or absolute. If we can't find a config file either
    # we default to a file open8055.users that is where this script is
    # installed.
    # ----
    if users_file is None:
        config = ConfigParser.SafeConfigParser()
        config_dir = None
        config.add_section('General')
        config.set('General', 'users_file', 'open8055.conf')
        for fname in config_locations:
            if not os.path.exists(fname):
                continue
            try:
                config.read(fname)
            except Exception as err:
                sys.stderr.write(fname + ': ' + str(err) + '\n')
                return 2
            config_dir = os.path.dirname(os.path.realpath(fname))
            break
        fname = config.get('General', 'users_file')
        if not os.path.isabs(fname):
            if config_dir is None:
                config_dir = os.path.dirname(os.path.realpath(__file))
            fname = os.path.join(config_dir, fname)

    fname = os.path.realpath(fname)
    tmpname = fname + '.tmp.' + str(os.getpid())

    # ----
    # If we are supposed to add or update a user, ask if we also
    # should change the password.
    # ----
    if flag_mode in (None, 'add', 'update', ):
        print 'Please enter the new password for user {0}'.format(username)
        print '(leave empty for keeping the existing password)'
        password = getpass.getpass()
        if password == '':
            password = None
        else:
            password = 'md5' + hashlib.md5(password).hexdigest()

    # ----
    # Open the two files
    # ----
    try:
        infile = open(fname, 'r')
    except Exception as err:
        sys.stderr.write('{0}: {1}\n'.format(fname, str(err)))
        return 3
    try:
        mode = stat.S_IRUSR | stat.S_IWUSR
        old_umask = os.umask(~mode & 0o777)
        outfile = open(tmpname, 'w')
        os.umask(old_umask)
    except Exception as err:
        sys.stderr.write('{0}: {1}\n'.format(tmpname, str(err)))
        return 3

    # ----
    # Perform the requested modification
    # ----
    user_found = False
    try:
        for line in infile.readlines():
            fuser, fisadmin, fpass = line.strip().split(':')
            if fuser != username:
                outfile.write(line)           
                continue
            user_found = True
            if flag_mode == 'add':
                raise Exception('Error: user ' + username + ' already exists')
            if flag_mode == 'delete':
                continue
            if flag_mode == 'update' or flag_mode is None:
                if flag_admin is not None:
                    fisadmin = flag_admin
                if password is not None:
                    fpass = password
                outfile.write(fuser + ':' + str(fisadmin) + ':' + fpass + '\n')
            
        if user_found is False:
            if flag_mode == 'update' or flag_mode == 'delete':
                raise Exception('Error: user ' + username +
                        ' does not exist')
            if password is None:
                raise Exception('Error: no password specified for ' +
                        'new user ' + username)
            if flag_admin is None:
                flag_admin = False

            outfile.write(username + ':' + str(flag_admin) + ':' +
                    password + '\n')
            
        infile.close()
        outfile.close()
        if os.name != 'posix':
            os.remove(fname)
        os.rename(tmpname, fname)

    except Exception as err:
        sys.stderr.write(str(err) + '\n')
        infile.close()
        outfile.close()
        os.remove(tmpname)
        return 4

    return 0
    

def usage():
    sys.stderr.write("""usage: {0} [OPTIONS] USERNAME

    Add, update or delete a user from the global users file of the
    open8055server.

Options:
    -A, --admin             make the user an administrator
    -N, --noadmin           make the user unprivileged (default for new users)
    -a, --add               add a new user (raise an error if the user exists)
    -d, --delete            delete an existing user
    -u, --update            update an existing user (raise and error if user
                            does not exist)
    -c, --config=PATH       location of open8055.conf file where to find the 
                            path of users file
    -f, --usersfile=PATH    location of the users file
    -h, --help              print this message
\n""".format(os.path.basename(__file__)))
    

if __name__ == '__main__':
    if os.name == 'posix':
        config_locations = ['/usr/local/etc/open8055.conf']
        config_locations.append(os.path.join(
                os.path.dirname(os.path.realpath(__file__)), 'open8055.conf'))
        sys.exit(main(sys.argv[1:], config_locations))

    else:
        sys.stderr.write('unsupported OS type ' + os.name + '\n')
