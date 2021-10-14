
/* quit.c - clean things up on exit. */
 
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "quit.h"
#include "config.h"
#include "lock.h"
#include "mbox.h"

/* Remove a temporary file, if it exists */

void cleartmp(char *fname)
{
	struct stat fst;

	if (!stat(fname, &fst)) 
		if (remove(fname))
			printf("Ouch, couldn't remove %s.\n", fname);
}

/* Update system and user mailbox
   (yo2loj - 13 april 2003) */
 
int save_mbox(void)
{
	FILE *mb;
	FILE *nm;
	FILE *ub;
	FILE *tmp;
	FILE *dest;
	
	long mblen=0;
	int havenew = 0;
	
	char buf[LINESIZE];
	int count;
		
	int i, line;
	int inhdr;
	char *status;
	int hasstatus;
	
	mb = fopen(mailbox, "r");
	/* Segfault protection if mailbox file doesn't exist. */
	if(mb == NULL) {
		printf("***\nSystem error, I can't see mail files.\nPlease inform your sysop - Error 1.\n***\n");
                return -1;
                }

	if (lock_fd(fileno(mb)))
		return -1;

	fseek(mb, 0, SEEK_END);
	mblen = ftell(mb);
	
	if (mblen < sysboxlen) {
	    printf("Mailbox changed by another program.\nMailbox not saved.\n");
	    unlock_fd(fileno(mb));
	    fclose(mb);
	    return -1;
	}
	
	if (mblen > sysboxlen) {
	    printf("New mail has arrived.\n");
	    havenew = 1;
	    
	    /* copy new mail to a temp file, will append to system mailbox later */
	    
	    if ((nm =  fopen(tempNewMail, "w+")) == NULL)
		panic("save_mbox: Could not create temporary file");

	    fseek(mb, sysboxlen, SEEK_SET);
	    
	    while (fgets(buf, LINESIZE, mb) != NULL) {
		count = strlen(buf);
		fwrite(buf, sizeof *buf, count, nm);
		if (ferror(nm))
		    panic("save_mbox: Could not write temporary file");
	    }
	    
	}
	
	if ((tmp = fopen(tempMail, "r")) == NULL)
	    panic("save_mbox: Could not open temporary mail file");
	    
	if (freopen(mailbox, "w", mb) == NULL)
	    panic("save_mbox: Could not open system mailbox for writing");
	    
	if ((ub = fopen(userbox, "w")) == NULL) {
	    /* error on user mailbox,
	       dump all to the system mailbox as last resort */
	       
	    while (fgets(buf, LINESIZE, tmp) != NULL) {
		count = strlen(buf);
		fwrite(buf, sizeof *buf, count, mb);
		if (ferror(mb))
		    panic("save_mbox: Could not write to system mailbox.");
	    }
	    fclose(tmp);
	    if (havenew) {
		fseek(nm, 0, SEEK_SET);
		while (fgets(buf, LINESIZE, nm) != NULL) {
		    count = strlen(buf);
		    fwrite(buf, sizeof *buf, count, mb);
		    if (ferror(mb))
			panic("save_mbox: Could not write to system mailbox.");
		}
		fclose(nm);
	    }
	    unlock_fd(fileno(mb));
	    fclose(mb);
	    printf("Could not open user mailbox, dumping to system mailbox\n");
	    return -1;
	}
	
	/* 
	   we have all processed messages in tmp and
	   any newly arrived (if any) in nm.
	   System mailbox is locked, empty and opened for write (mb),
	   User Mailbox is empty and opened for writing (ub).
	   
	   	- read message go to ~/mbox with status RO
		- not read go back to system mailbox with status O
		- deleted are purged
		- any arrived go to system mailbox as they are
	*/
	
	for (i=0; i<messages; i++) {
	    dot = &message[i];	
	    if (fseek(tmp, dot->m_offset, SEEK_SET)) {
		printf("save_mbox: Could not find message %i.\n", i);
		continue;
	    }
	    
	    if (dot->m_flag & MDELETED) {
		dest = NULL;
		status = NULL;
	    }
	    else if (dot->m_flag & MREAD) {
		dest = mb;
		status = "RO";
	    }
	    else if (dot->m_flag & MNEW) {
		dest = mb;
		status = "O";
	    }
	    else {
		dest = mb;
		status = NULL;
	    }
	    
	    line = 0;
	    hasstatus = 0;
	    inhdr = 1;
	    
	    for (;;) {
		if (!dest) break;
		if (fgets(buf, LINESIZE, tmp) == NULL) {
		    /* End of file */
		    break;
		}
		line ++;
		if (inhdr) {
		    /* Header */
		    if (!strncasecmp(buf, "Status: ", 8)) {
			hasstatus = 1;
			if (status) sprintf(buf, "Status: %s\n", status);
		    } else if (!strcmp("\n", buf)) {
			inhdr = 0;
			if (!hasstatus) {
			    if (status) sprintf(buf, "Status: %s\n", status);
			    fwrite(buf, sizeof *buf, strlen(buf), dest);
			}
			strcpy(buf, "\n");
		    }
		} else {
		    /* Data */
		    if (line == dot->m_lines + 1) break;
		}
		fwrite(buf, sizeof *buf, strlen(buf), dest);
	    }
	}
	
	fclose(tmp);
		
	if (havenew) {
	    /* append the newly arrived mail */
	    fseek(nm, 0, SEEK_SET);
	    freopen(mailbox, "a+", mb);
	    while (fgets(buf, LINESIZE, nm) != NULL) {
		count = strlen(buf);
		fwrite(buf, sizeof *buf, count, mb);
		if (ferror(mb))
		    panic("save_mbox: Could not write to system mailbox.");
	    }
	    
	    fclose(nm);
	}
	
	unlock_fd(fileno(mb));
	fclose(mb);
	fclose(ub);

	return 0;
}

/* On quit without save: check and announce if new mail has arrived */
 
int chk_new_msg(void)
{
	FILE *mb;
	long mblen=0;
	
	mb = fopen(mailbox, "r");
	if (lock_fd(fileno(mb)))
		return -1;

	fseek(mb, 0, SEEK_END);
	mblen = ftell(mb);
	
	if (mblen != sysboxlen) printf("New mail has arrived.\n");

	unlock_fd(fileno(mb));
	fclose(mb);

	return 0;
}

/* quit. */

void quit(int save, int code)
{
	if (save) {
		printf("Exiting, saving changes in mailbox.\n");
	} else
		printf("Cancelling changes, mailbox not changed.\n");
	if (save) {
		save_mbox();
	}
	else
		chk_new_msg();
	printf("Bye %s!.. from axMail@%s.\n", username, hostname);	
	cleartmp(tempMail);
	cleartmp(tempNewMail);
	cleartmp(tempEdit);
	cleartmp(tempMesg);
	
	exit(code);
}

