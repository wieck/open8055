@echo off
rem ############################################################
rem # Script to install, remove, start, stop or restart the
rem # open8055server service on Windows.
rem ############################################################

    goto main

:install
	echo installing open8055server service
	open8055server.py install
	sc config open8055server start= auto
	net start open8055server
	goto end

:remove
	echo removing open8055server service
	net stop open8055server
	open8055server.py remove
	goto end

:start
	net start open8055server
	goto end

:stop
	net stop open8055server
	goto end

:restart
	net stop open8055server
	net start open8055server
	goto end

:main
	if "%1" == "install" goto install
	if "%1" == "remove" goto remove
	if "%1" == "start" goto start
	if "%1" == "stop" goto stop
	if "%1" == "restart" goto restart

	echo usage: open8055service {install/remove/start/stop/restart}

:end
