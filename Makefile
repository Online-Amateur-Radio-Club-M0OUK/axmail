all: axmail

MAN_DIR = /usr/local/share/man
CC = gcc
LD = gcc
CFLAGS = -O2 -Wstrict-prototypes -g -I../lib
LIBS = -lcrypt
MODULES = utils.o config.o adduser.o command.o mailcmd.o mbox.o head.o lock.o axmail.o quit.o

.c.o:
	$(CC) $(CFLAGS) -c $<

upgrade: installbin installhelp installman

install: installbin installconf installhelp installman

installbin: all
	install -m 0755 -s -o root -g root axmail	 /usr/local/sbin

installconf:
	install -m 755    -o root -g root -d		/usr/local/etc/ax25
	install -b -m 644 -o root -g root etc/axmail.conf /usr/local/etc/ax25
	install -m 644    -o root -g root etc/welcome.txt /usr/local/etc/ax25

installhelp:
	install -m 755    -o root -g root -d		  /usr/local/var/lib/ax25/axmail/help
	install -m 644    -o root -g root etc/help/*.hlp  /usr/local/var/lib/ax25/axmail/help

installman:
	install -m 644    -p man/axmail.8	$(MAN_DIR)/man8
	install -m 644    -p man/axmail.conf.5	$(MAN_DIR)/man5
back:
	rm -f ../mail.tar.gz
	tar cvf ../mail.tar *
	gzip ../mail.tar

clean:
	rm -f axmail *.o *~ *.bak core etc/*~ etc/help/*~

distclean: clean
	rm -f axmail

axmail: $(MODULES)
	$(LD) -o axmail $(MODULES) $(LIBS) $(LDFLAGS)

utils.o:	utils.h utils.c mbox.h
config.o:	config.h config.c defines.h axmail.h utils.h
adduser.o:	adduser.h adduser.c utils.h config.h defines.h
command.o:	command.h command.c config.h mailcmd.h mbox.h utils.h quit.h
mailcmd.o:	mailcmd.h mailcmd.c defines.h utils.h mbox.h config.h
mbox.o:		mbox.h mbox.c utils.h defines.h config.h head.h
head.o:		head.h head.c defines.h
utils.o:	utils.h utils.c 
lock.o:		lock.h lock.c utils.h
quit.o:		quit.h quit.c config.h lock.h
axmail.o:	axmail.h axmail.c config.h adduser.h utils.h quit.h mbox.h
