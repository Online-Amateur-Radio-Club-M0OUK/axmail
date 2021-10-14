
/*
 *	mbox.c - Read, parse and write the system mailbox
 *
 *	Much taken from mailx-5.3b, which is 
 *	Copyright (c) 1980 Regents of the University of California.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "mbox.h"
#include "utils.h"
#include "defines.h"
#include "config.h"
#include "head.h"

FILE *mbox;			/* Temporary mailbox in /tmp */
struct message *message;	/* Message list */
struct message *dot;		/* Current message pointer */
int messages = 0;		/* Amount of messages in list */
int current = 0;		/* Current message index */
int newm = 0;			/* New messages */

long sysboxlen = 0;		/* Length of the system mailbox at reading */
				/* It will increase if new arrives */

/* Check if we have this message in the message list yet */

int havemessage(char *id)
{
	int i;
	
	for (i = 0; i < messages; i++) {
		if (!strcmp(message[i].id, id))
			return 1;
	}
	
	return 0;
}

/* Open a temp file by creating and unlinking.
   Return the open file descriptor. */

int opentemp(char file[])
{
	int f;
	
	if ((f = open(file, O_CREAT|O_EXCL|O_RDWR, 0600)) < 0)
		panic("opentemp: could not open file");
	remove(file);
	return f;
}

/* Take the data out of the passed ghost file and toss it into
   a dynamically allocated message structure. */

void makemessage(FILE *f)
{
	int size = (messages + 1) * sizeof (struct message);

	if (message != 0)
		free((char *) message);
	if ((message = (struct message *) malloc((unsigned) size)) == 0)
		panic("Insufficient memory for %d messages", messages);
	dot = message;
	size -= sizeof (struct message);
	fflush(f);
	lseek(fileno(f), (long) sizeof *message, 0);
	if (read(fileno(f), (char *) message, size) != size)
		panic("Message temporary file corrupted");
	message[messages].m_size = 0;
	message[messages].m_lines = 0;
	message[messages].receipt = NULL;
	message[messages].from = NULL;
	message[messages].date = NULL;
	message[messages].subj = NULL;
	message[messages].id = NULL;
	fclose(f);
}

/* Append the passed message descriptor onto the temp file.
   If the write fails, return 1, else 0 */

void append(struct message *mp, FILE *f)
{
	if (fwrite((char *) mp, sizeof *mp, 1, f) != 1)
		panic("append: Could not append message");
}

/* Copy mailbox from ~/mail/mbox and /var/spool/mail to /tmp
   (yo2loj - 13 april 2003) */

int getmail(void)
{
	struct stat stb;

	FILE *ibuf;
	FILE *mestmp;

	char buf[LINESIZE];
	int count;

	long offset = 0;
	int inheader = 0;
	int maybe = 1;
	char *cp = NULL;
	char c;


	struct message this = { MUSED|MNEW, 0, 0, 0 };
	
	
	if ((mbox = fopen(tempMail, "w")) == NULL)
		panic("getmail: Could not create temporary file");

	
	if ((ibuf = fopen(userbox, "r")) == NULL) goto sysbox;
	
	if (fstat(fileno(ibuf), &stb) < 0) {
		printf("Ouch, could not fstat() mailbox.\n");
		syslog(LOG_NOTICE, "getmail: Could not fstat()");
		fclose(ibuf);
		goto sysbox;;
	}
	
	switch (stb.st_mode & S_IFMT) {
		case S_IFREG:		/* Good, a regular file */
			break;
		default:		/* Yuck, i won't eat THAT! */
			fclose(ibuf);
			goto sysbox;
	}
	
	if (stb.st_size == 0) {		/* Zero-sized mailbox? */
		fclose(ibuf);
		goto sysbox;;
	}
	
	/* copy old read mail */
	
	while (fgets(buf, LINESIZE, ibuf) != NULL) {
	    count = strlen(buf);
	    fwrite(buf, sizeof *buf, count, mbox);
	    if (ferror(mbox))
			panic("getmail: Could not write temporary file");	
	}

	fclose(ibuf);
	
sysbox:
	if ((ibuf = fopen(mailbox, "r")) == NULL) goto rdexit;
	
	if (fstat(fileno(ibuf), &stb) < 0) {
		printf("Ouch, could not fstat() mailbox.\n");
		syslog(LOG_NOTICE, "getmail: Could not fstat()");
		fclose(ibuf);
		goto rdexit;
	}
	
	switch (stb.st_mode & S_IFMT) {
		case S_IFREG:		/* Good, a regular file */
			break;
		default:		/* Yuck, i won't eat THAT! */
			fclose(ibuf);
			goto rdexit;
	}
	
	if (stb.st_size == 0) {		/* Zero-sized mailbox? */
		fclose(ibuf);
		goto rdexit;
	}
	
	/* copy unread and new mail */
	
	while (fgets(buf, LINESIZE, ibuf) != NULL) {
	    count = strlen(buf);
	    fwrite(buf, sizeof *buf, count, mbox);
	    sysboxlen += count;
	    if (ferror(mbox))
			panic("getmail: Could not write temporary file");
	}
	
	fclose(ibuf);

rdexit:
	fclose(mbox);	/* all mail is now in temp file - digest it! */

	if ((c = opentemp(tempMesg)) < 0)
		exit(1);
		
	if ((mestmp = fdopen(c, "r+")) == NULL)
		panic("getmail: Could not open temporary file");

	if ((mbox = fopen(tempMail, "r")) == NULL)
		panic("getmail: Could not open temporary file");


	for (;;) {
		if (fgets(buf, LINESIZE, mbox) == NULL) {
		    /* End of file */
		    append(&this, mestmp);
		    makemessage(mestmp);
		    if (fclose(mbox))
		    	panic("getmail: Could not close temporary mailbox");
		    return 0;
		}
		count = strlen(buf);
		buf[count - 1] = '\0';
		if (maybe && buf[0] == 'F' && ishead(buf)) {
			messages++;
			newm++;
			append(&this, mestmp);
			this.m_flag = MUSED|MNEW;
			this.m_size = 0;
			this.m_lines = 0;
			this.m_offset = offset;
			this.receipt = NULL;
			this.subj = NULL;
			this.from = NULL;
			this.date = NULL;
			this.id = NULL;
			inheader = 1;
		} else if (buf[0] == 0) {
			inheader = 0;
		} else if (inheader) {
			if (!strncasecmp(buf, "Status: ", 8)) {
				cp = &buf[8];
				if (strchr(cp, 'R') != NULL)
					this.m_flag |= MREAD;
				if (strchr(cp, 'O') != NULL) {
					this.m_flag &= ~MNEW;
					newm--;
				}
			}
			
                        if ((!strncasecmp(buf, "Disposition-Notification-To:", 28)) && (this.receipt == NULL)) {
                                cp = &buf[28];
                                while ((*cp) && (isspace(*cp)))
                                        cp++;
                                this.receipt = strdup(cp);
                        }

			if ((!strncasecmp(buf, "Subject:", 8)) && (this.subj == NULL)) {
				cp = &buf[8];
				while ((*cp) && (isspace(*cp)))
					cp++;
				this.subj = strdup(cp);
			}
			
			if ((!strncasecmp(buf, "From:", 5)) && (this.from == NULL)) {
				cp = &buf[5];
				while ((*cp) && (isspace(*cp)))
					cp++;
				this.from = strdup(cp);
			}
			
			if ((!strncasecmp(buf, "Date:", 5)) && (this.date == NULL)) {
				cp = &buf[5];
				while ((*cp) && (isspace(*cp)))
					cp++;
				this.date = strdup(cp);
			}

			if ((!strncasecmp(buf, "Message-ID:", 11)) && (this.id == NULL)) {
				cp = &buf[11];
				while ((*cp) && (isspace(*cp)))
					cp++;
				this.id = strdup(cp);
			}
		}
		offset += count;
		this.m_size += count;
		this.m_lines++;
		maybe = buf[0] == 0;

	}
	
	
	return 0;
}

/* Read a message */
 
int readmesg(int msg, int verbose)
{
	int i, line = 0;
	int inhdr = 1;
	char buf[LINESIZE];
	if ((msg < 1) || (msg > messages)) {
		printf("There's no message %i.\n", msg);
		return -1;
	}

	if ((mbox = fopen(tempMail, "r")) == NULL) {
		printf("Could not open temporary mailbox.\n");
		return -1;
	}

	i = msg - 1;
	dot = &message[i];

	if (fseek(mbox, dot->m_offset, SEEK_SET)) {
		printf("readmesg: Could not fseek to message %i.\n", msg);
		return -1;
	}
	
	current = msg;
	
	printf("Message %i:%s%s%s\n", msg, 
		((dot->m_flag & (MREAD|MNEW)) == MNEW) ? " (New)" : "",
		((dot->m_flag & (MREAD|MNEW)) == 0) ? " (Unread)": "",
		((dot->m_flag & (MDELETED)) == MDELETED) ? " (Deleted)" : ""
		);
		
	dot->m_flag |= MREAD;
	dot->m_flag ^= (dot->m_flag & MNEW);
	
	for (;;) {
		if (fgets(buf, LINESIZE, mbox) == NULL) {
			/* End of file */
			fclose(mbox);
			return 0;
		}
		line++;
		if (inhdr) {
			/* Headers */
			if (((verbose) && (line != 1) && (strncasecmp(buf, "Status: ", 8)) && (strncasecmp(buf, "X-Status: ", 10)) ) ||
			    (!strncasecmp(buf, "Date:", 5)) ||
			    (!strncasecmp(buf, "From:", 5)) ||
			    (!strncasecmp(buf, "To:", 3)) ||
			    (!strncasecmp(buf, "Cc:", 3)) ||
//			    (!strncasecmp(buf, "Disposition-Notification-To:", 3)) ||
			    (!strncasecmp(buf, "Subject:", 8)))
				printf("%s", buf);

			if (!strcmp("\n", buf)) {
				inhdr = 0;
			}
		} else {
			/* Data */
			if (line == dot->m_lines) {
				fclose(mbox);
				return 0;
			}
			printf("%s", buf);
		}

	}
	return 0;

}
