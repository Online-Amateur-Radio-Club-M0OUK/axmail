/* axmail.c - The command parser and main function */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>
#include <pwd.h>
#include <crypt.h>
#include <error.h>
#include <grp.h>

#include "config.h"
#include "axmail.h"
#include "utils.h"
#include "adduser.h"
#include "quit.h"
#include "mbox.h"

/* Parse c-style escapes (neat to have!) */
 
// int setgroups(void);

static char *parse_string(char *str)
{
	char *cp = str;
	unsigned long num;

	while (*str != '\0' && *str != '\"') {
		if (*str == '\\') {
			str++;
			switch (*str++) {
			case 'n':
				*cp++ = '\n';
				break;
			case 't':
				*cp++ = '\t';
				break;
			case 'v':
				*cp++ = '\v';
				break;
			case 'b':
				*cp++ = '\b';
				break;
			case 'r':
				*cp++ = '\r';
				break;
			case 'f':
				*cp++ = '\f';
				break;
			case 'a':
				*cp++ = '\007';
				break;
			case '\\':
				*cp++ = '\\';
				break;
			case '\"':
				*cp++ = '\"';
				break;
			case 'x':
				num = strtoul(--str, &str, 16);
				*cp++ = (char) num;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				num = strtoul(--str, &str, 8);
				*cp++ = (char) num;
				break;
			case '\0':
				return NULL;
			default:
				*cp++ = *(str - 1);
				break;
			};
		} else {
			*cp++ = *str++;
		}
	}
	if (*str == '\"')
		str++;		/* skip final quote */
	*cp = '\0';		/* terminate string */
	return str;
}

/* Parse command line to argv, honoring quotes and such */
 
int parse_args(char *argv[],char *cmd)
{
	int ct = 0;
	int quoted;

	while (ct < 31)
	{
		quoted = 0;
		while (*cmd && isspace(*cmd))
			cmd++;
		if (*cmd == 0)
			break;
		if (*cmd == '"') {
			quoted++;
			cmd++;
		}
		argv[ct++] = cmd;
		if (quoted) {
			if ((cmd = parse_string(cmd)) == NULL)
				return 0;
		} else {
			while (*cmd && !isspace(*cmd))
				cmd++;
		}
		if (*cmd)
			*cmd++ = 0;
	}
	argv[ct] = NULL;
	return ct;
}

/* Find the command from the command table and execute it. */
 
int cmdparse(struct cmd *cmds, char *cmdline)
{
	struct cmd *cmdp;
	int argc;
	char *argv[32];

	if ((argc = parse_args(argv, cmdline)) == 0 || *argv[0] == '#')
		return 0;
	strlwr(argv[0]);
	for (cmdp = cmds; cmdp->function != NULL; cmdp++)
		if (strncasecmp(cmdp->name, argv[0], strlen(argv[0])) == 0)
			break;
	if (cmdp->function == NULL)
		return -1;
	return (*cmdp->function)(argc, argv);
}

/* Signal handlers (which don't do all the things they SHOULD do) */

static void alarm_handler(int sig)
{
	printf("\nTimeout! Closing...\n");
	exit(1);
}

static void term_handler(int sig)
{
	printf("System going down! Closing...\n");
	exit(1);
}

/* Initialisation for the user */
 
int init_user(char *call)
{
	struct passwd *pw;
	char pass[13], salt[3];
	char *p;
	char axhome[64];

	/* Strip SSID */
	if (local) {
		pw = getpwuid(getuid());
	} else {
		strcpy(callsign, call);
		strcpy(username, callsign);
		strlwr(username);
		p = strchr(username, '-');
		if (p) *p = '\0';
		pw = getpwnam(username);
	}

	if (!pw) {
		if (local)
			panic("Ouch, you don't seem to be in the password file.\n");
		if (autocreate) {
			syslog(LOG_NOTICE, "Adding new user %s", username);
			if (new_user(username) < 0) {
				syslog(LOG_NOTICE, "Could not add new user %s", username);
				printf("Sorry, could not add new user.\n");
				return -1;
			}
			printf("You have been added to the user list of %s.\nWelcome.\n", hostname);
			pw = getpwnam(username);
		} else {
			syslog(LOG_NOTICE, "New user %s not accepted", username);
			printf("Sorry, new users are not accepted at this time.\n");
			return -1;
		}
	} else {
		if (local) {
			strcpy(username, pw->pw_name);
			strcpy(callsign, username);
		}
		/* Strip full name from the gecos field... */
		if (strchr(pw->pw_gecos, ',') == NULL)
			strcpy(fullname, pw->pw_gecos);
		else
			strcpy(fullname, strtok(pw->pw_gecos, ","));
	}
	
	if (!local) {
		if ((mail_allowed == 1) && (strcmp(pw->pw_passwd, "*"))) {
			printf("Sorry, you are not allowed to use axmail (you have a password set).\n");
			return -1;
		}
		
		if ((mail_allowed == 2) && (!strcmp(pw->pw_passwd, "*"))) {
			printf("Sorry, you are not allowed to use axmail (locked out in password file).\n");
			return -1;
		}
		if (mail_allowed == 3) {
			sprintf(axhome, "%s/%s", def_homedir, username);
			if (strcmp(pw->pw_dir, axhome)) {
				printf("Sorry, you are not allowed to use axmail - bad axhome.\n");
				return -1;
			}
		}
		
		if (identification == 1) {
			getstr(pass, 12, "Password: ");
			strncpy(salt, pw->pw_passwd, 2);
			salt[2] = '\0';
			if (strcmp(pw->pw_passwd, (char *)crypt(pass, salt))) {
				printf("Login incorrect.\n");
				return -1;
			}
		}

/* code supplied by Jaroslav Skarvada */
		if ( (setgroups(0, NULL) == -1) || (setgid(pw->pw_gid) == -1) || (setuid(pw->pw_uid) == -1) )
			panic("init_user: Argh, cannot setuid() or setgid() to %i.%i", pw->pw_uid, pw->pw_gid);
	}
	
	homedir = strdup(pw->pw_dir);
	return 0;
}

int main(int argc, char **argv)
{
	char *p;

	signal(SIGALRM, alarm_handler);
	signal(SIGTERM, term_handler);
        openlog("axmail", LOG_PID, LOG_DAEMON);

	if (getuid() != 0)
		local = 1;	/* Hey, we're being executed by a "normal"
				 * user, with user privileges! 
				 */
	
	if ((!local) && (argc != 2)) {
		printf("axmail: Callsign not found as a parameter.\n");
		syslog(LOG_NOTICE, "Callsign not found as a parameter\n");
		return 1;
	}
	
	if (read_config() == -1) /* Read axmail.conf */
		return 1;
		
	printf("%s\n", VERSION);
	
	if (init_user(argv[1]) == -1) /* Get user specific data, create user account */
		return 1;
		
	tinit();	/* Filenames etc */
	getmail();	/* Scan mailbox */
	
	if ((newm) || (messages))
		printf("You have %i messages (%i new).\n", messages, newm);
		
	prompt();
	fflush(stdout);
	
	p = malloc(1024);
	
	while (1)
	{
		fgets(p, 1023, stdin);

		if (p == NULL)
                	quit(0, 0);
                	
		if (cmdparse(Mailcmds, p) == -1)
			printf("Unknown command. Type ? for a list.\n");
		
		prompt();
		fflush(stdout);
	}
	
}

