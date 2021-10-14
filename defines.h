
 /*
  *	defines.h - Compile-time configuration
  */

#define VERSION "axMail-Fax v2.13"
#define COPYRIGHT "(c) 1996-1998 Heikki Hannikainen (OH7LZB) <hessu@hes.iki.fi>\nMailbox save support (c) 2003 Marius Petrescu (YO2LOJ) <mpetrescu@online.ro>\nOther modifications (c) 2005-2021 by Brian Rogers (N1URO) <n1uro@n1uro.net> "
#define PROMPT "=> "

#define CONF_AXMAIL_FILE "/usr/local/etc/ax25/axmail.conf"
#define CONF_AXMAIL_PROF_FILE "/usr/local/etc/ax25/ax25.profile"
// #define CONF_AXMAIL_USER_FILE ".axmailrc"
#define DATA_AXMAIL_HELP_DIR "/usr/local/var/lib/ax25/axmail/help/"
#define DATA_AXMAIL_MAIL_DIR "/usr/local/var/spool/mail/"
#define LOCK_AXMAIL_FILE "/var/lock/axmail"
#define BIN_AXMAIL_SENDMAIL "/usr/sbin/sendmail"


#define FORWARDFILE ".forward"
#define SIGNATUREFILE ".signature"
#define USERPROFILE ".profile"
#define PASSWDFILE "/etc/passwd"

#define WELCOME "/usr/local/etc/ax25/welcome.txt"

#define PATHSIZE 1024
#define LINESIZE 1024	/* Maximum length of a line in a message */

#define TRUE 1
#define FALSE 0

#define LICENSE "\n   This program is free software; you can redistribute it and/or modify\n  it under the terms of the GNU General Public License as published by\n  the Free Software Foundation; either version 2 of the License, or\n  (at your option) any later version. Usage may vary based on location.\n\n"
