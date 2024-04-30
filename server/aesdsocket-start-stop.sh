#! /bin/sh

case "$1" in
	start)
		echo "Starting aesd server"
		start-stop-daemon -S -n aesdsocket /usr/bin/aesdsocket -- -d
		;;
	stop
		echo "stopping aesd server"
		start-stop daemon -K -n aesdsocket
		;;
	*)
		echo "Useage: $0 {start | stop}"
	exit 1
esac

exit 0