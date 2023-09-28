apt-get update && apt-get upgrade -y
apt-get install inspircd -y
apt-get install tcpflow -y
apt-get install irssi -y

mkdir /var/run/inspircd

inspircd --runasroot --nofork

# in other tab
tcpflow -i lo port 6667 -c

# in other tab
# connect to ircserv
irssi -c 10.13.5.3 -p 6667 -w 1234 -n subcho
# connect to inspircd
irssi -c 127.0.0.1 -p 6667 -n yeselee