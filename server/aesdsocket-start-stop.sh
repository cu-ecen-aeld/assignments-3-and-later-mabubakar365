#!/bin/bash
case "$1" in
start)
echo "Starting aesdsocket"
start-stop-daemon -S -n aesdsocket -a /home/embeddedmaster/Courses/Linux/Week4/assignments-3-and-later-mabubakar365/server/aesdsocket -- -d 
;;
stop)
echo "Stopping aesdsocket"
start-stop-daemon -K SIGTERM -n aesdsocket 
;;
*)
echo "Usage: $0 {start|stop}"
exit 1
esac
exit 0
