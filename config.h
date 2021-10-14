#include <utmp.h>
#include "defines.h"

extern char callsign[20];
extern char username[20];
extern char fullname[31];
extern char forward[79];

extern char *homedir;		/* User's home directory */
extern char *fwdfile;		/* User's .forward file (~/.forward) */
extern char *sigfile;		/* User's .signature file (~/.signature) */
extern char *maildir;		/* User's mail directory (~/mail) */
extern char *mailbox;		/* System mailbox (/var/spool/mail/user) */
extern char *userbox;		/* User's mailbox (~/mbox) */
extern char *mailconf;		/* User's own axmail configuration */

extern int local;		/* Running with user privileges? */

extern char *tempMail; 		/* Temporary files: ~/mail/ax?(-pid-) */
extern char *tempRcpt;		/* Temporary files: return receipt */
extern char *tempNewMail;	/* Temporary files: ~/mail/axnew?(-pid-) */
extern char *tempEdit;
extern char *tempMesg;

extern char *faxgate;		/* Email of your E-Fax Gateway */
extern char *hostname;
extern char *sysopmail;
extern char *def_shell;		/* Default settings for autoaccounts */
extern char *def_homedir;

extern int mail_allowed, identification, autocreate, login_allowed;
extern int first_uid, last_uid;
extern gid_t user_gid;
extern char mboxname[PATHSIZE];
extern long IdleTimeout;

struct cmd {
	char	*name;
	int	(*function)	(int argc, char **argv);
};

extern struct cmd Mailcmds[];

extern void tinit(void);
extern int read_config(void);
