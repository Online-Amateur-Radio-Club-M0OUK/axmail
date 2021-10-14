
/* config.c - read configuration */
  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/stat.h>

#include "config.h"
#include "defines.h"
#include "axmail.h"
#include "utils.h"

char callsign[20];
char username[20];
char fullname[31];
char *homedir;			/* User's home directory */
char *maildir;			/* User's mail directory (~/mail) */
char *mailbox;			/* System mailbox (/var/spool/mail/user) */
char *userbox;			/* User's mailbox (~/mbox) */
char *sysopmail;		/* Sysop's email address */
char *mailconf;			/* User's own axmail configuration */

int local = 0;			/* Running with user privileges? */

char *tempMail; 		/* Temporary files: ~/mail/ax?(-pid-) */
char *tempNewMail;		/* Temporary files: ~/mail/axnew?(-pid-) */
char *tempEdit;
char *tempMesg;
char *tempRcpt;

char *hostname		= NULL;
char *def_shell		= NULL;
char *def_homedir	= NULL;
char *faxgate		= NULL;

int mail_allowed	= 0;
int identification	= 0;

int autocreate 		= FALSE;
int login_allowed	= FALSE;
int first_uid		= 400;
int last_uid		= 65535;
gid_t user_gid		= 0;

long IdleTimeout	= 900L;		/* default to 15 mins */

static int do_allowmail(int, char **);
static int do_autocreate(int, char **);
static int do_autogroup(int, char **);
static int do_first_uid(int, char **);
static int do_homedir(int, char **);
static int do_faxgate(int, char **);
static int do_hostname(int, char **);
static int do_sysopmail(int, char **);
static int do_identification(int, char **);
static int do_idletimeout(int, char **);
static int do_loginallowed(int, char **);
static int do_last_uid(int, char **);
static int do_shell(int, char **);

static struct cmd cfg_cmds[] = {
	{ "allowmail",		do_allowmail	},
	{ "autocreate",		do_autocreate	},
	{ "autogroup",		do_autogroup	},
	{ "faxgate",		do_faxgate	},
	{ "first_uid",		do_first_uid	},
	{ "homedir",		do_homedir	},
	{ "hostname",		do_hostname	},
	{ "identification",	do_identification	},
	{ "idletimeout",	do_idletimeout	},
	{ "loginallowed",	do_loginallowed	},
	{ "last_uid",		do_last_uid	},
	{ "shell",		do_shell	},
	{ "sysopmail",		do_sysopmail	},
	{ NULL,			NULL		}
};

/* ***************************************************************** */

static int do_allowmail(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	if (!strcasecmp(argv[1], "all"))
		mail_allowed = 0;
	else if (!strcasecmp(argv[1], "nologin"))
		mail_allowed = 1;
	else if (!strcasecmp(argv[1], "passwd"))
		mail_allowed = 2;
	else if (!strcasecmp(argv[1], "axhome"))
		mail_allowed = 3;
	else
		return -1;
	return 0;
}

static int do_autocreate(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	if (!strcasecmp(argv[1], "yes"))
		autocreate = 1;
	else
		autocreate = 0;
	return 0;
}

static int do_autogroup(int argc, char **argv)
{
	if (argc < 2)
		return -1;

	user_gid = atoi(argv[1]);

        if (user_gid == 0) {
		struct group * gp = getgrnam(argv[1]);
		if (gp != NULL)
			user_gid = gp->gr_gid;
		else {
			printf("Group %s not found.\n", argv[1]);
			return -1;
		}
		endgrent();
	}

	if (user_gid == 0) {
		printf("Default GID for new users must be set and non-zero.\n");
		return -1;
	}
	return 0;
}

static int do_faxgate(int argc, char **argv)
{
	        if (argc < 2)
			                return -1;
		        faxgate = strdup(argv[1]);
			        return 0;
}

static int do_first_uid(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	first_uid = atoi(argv[1]);
	return 0;
}

static int do_last_uid(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	last_uid = atol(argv[1]);
	return 0;
}

static int do_idletimeout(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	IdleTimeout = atol(argv[1]);
	return 0;
}

static int do_homedir(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	def_homedir = strdup(argv[1]);
	return 0;
}

static int do_hostname(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	hostname = strdup(argv[1]);
	return 0;
}

static int do_sysopmail(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	sysopmail = strdup(argv[1]);
	return 0;
}

static int do_identification(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	if (!strcasecmp(argv[1], "none"))
		identification = 0;
	else if (!strcasecmp(argv[1], "passwd"))
		identification = 1;
	else
		return -1;
	return 0;
}

static int do_loginallowed(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	if (!strcasecmp(argv[1], "yes"))
		login_allowed = 1;
	else if (!strcasecmp(argv[1], "crazy"))
			login_allowed = 2;
		else
			login_allowed = 0;
	return 0;
}

static int do_shell(int argc, char **argv)
{
	if (argc < 2)
		return -1;
	def_shell = strdup(argv[1]);
	return 0;
}

/* ***************************************************************** */

/* Set the temporary file names, paths and such, and make sure
   they're available for us to use... */

void tinit(void)
{
	int pid;
	char pat[PATHSIZE];
	struct stat fst;

	umask(077);
	pid = getpid();
		
	sprintf(pat, "%s/mail", homedir);
	maildir = strdup(pat);
	
	sprintf(pat, "%s/axM%05d", maildir, pid);
	tempMail = strdup(pat);
	sprintf(pat, "%s/axnewM%05d", maildir, pid);
	tempNewMail = strdup(pat);
	sprintf(pat, "%s/axE%05d", maildir, pid);
	tempEdit = strdup(pat);
	sprintf(pat, "%s/axT%05d", maildir, pid);
	tempMesg = strdup(pat);
	
	sprintf(pat, "%s/mbox", homedir);
	userbox = strdup(pat);
	
	sprintf(pat, "%s%s", DATA_AXMAIL_MAIL_DIR, username);
	mailbox = strdup(pat);
	
//	sprintf(pat, "%s%s", homedir, CONF_AXMAIL_USER_FILE);
//	mailconf = strdup(pat);
	
	if (stat(maildir, &fst) < 0) {
		printf("Creating directory %s...\n", maildir);
		if (mkdir(maildir, S_IRUSR|S_IWUSR|S_IXUSR))
			panic("tinit: Cannot create mail directory");
	}
}

/* Read configuration */
 
int read_config(void)
{
	FILE *fp;
	char line[256];
	int ret, n = 0;

	if ((fp = fopen(CONF_AXMAIL_FILE, "r")) == NULL) {
		printf("Cannot read axmail.conf\n");
		return -1;
	}
	while (fgets(line, 256, fp) != NULL) {
		n++;
		ret = cmdparse(cfg_cmds, line);
		if (ret == -1)
			printf("Syntax error in config file at line %d: %s\n", n, line);
		if (ret < 0) {
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

