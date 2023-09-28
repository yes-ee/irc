docker run -d --name ubuntu -p 80:80 -it --privileged ubuntu:20.04

apt-get update && apt-get upgrade -y
apt-get install inspircd -y tcpflow -y irssi -y

mkdir /var/run/inspircd

inspircd --runasroot --nofork

# in other tab
tcpflow -i lo port 6667 -c

# in other tab
# connect to ircserv
irssi -c 10.12.4.3 -p 1012 -w 1234 -n yeselee
# connect to inspircd
irssi -c 127.0.0.1 -p 6667 -n subcho