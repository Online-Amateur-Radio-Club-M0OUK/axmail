
/* utils.c - common utilities */
 
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>

#include "utils.h"
#include "mbox.h"
#include "config.h"

//extern  int     sys_nerr;

/* Send the prompt */
 
void prompt(void)
{
	printf("%s@%s\nCurrent message %i of %i\n=> ", callsign, hostname, current, messages);
}

/* Prompt for email forwarding */
void getaddy(char *forward)
{	
	char email[79];
	getstr(email, 79, "What email address do you want to autoforward mail to? This must be a full\nvalid email address. Example: n1uro@n1uro.com or just hit ENTER to clear.\n");
	strcpy(forward, email);
}

/* Prompt for signature */
void getsig(char *signature)
{
        char email[79];
        getstr(email, 79, "Hit enter to remove or type your signature here:\n");
        strcpy(signature, email);
}


/* Prompt for name */

void getname(char *uname)
{
	char nam[81];

rename:
	getstr(nam, 80, "Enter your name (? for help): ");
	
	if (!strcmp(nam, "?")) {
		printf("The name entered will be used in the From: header of the messages you send.\n");
		printf("It will also be shown in finger queries and a number of other places.\n");
		printf("It is usually set to your full name, but you may also use a nickname\n");
		printf("your friends know.\n");
		goto rename;
	}

	if (strlen(nam) > 30) {
		printf("The length of the name is limited to 30 characters.\n");
		goto rename;
	}
	
	if (anyof(nam, "=<>()|:;!\\\",")) {
		printf("Characters = < > ( ) | : ; ! \\ \" , are not allowed.\n");
		goto rename;
	}
		
	if (anycntrls(nam)) {
		printf("Control characters are not allowed.\n");
		goto rename;
	}

	strcpy(uname, nam);
}

/* Get a string from the user, with maximum length limit */

void getstr(char *str, int len, char *prompt)
{
	char *p;

	printf("%s", prompt);
	fflush(stdout);
	fgets(str, len, stdin);

	/* Strip CR */
	p = strchr(str, '\n');
	if (p != NULL)
		strcpy(p, "");

}

/* Convert a string to upper case */

char *strupr(char *s)
{
	char *p;

	if (s == NULL)
		return NULL;
	
	for (p = s; *p != '\0'; p++)
		*p = toupper(*p);
		
	return s;
}

/* Convert a string to lower case */

char *strlwr(char *s)
{
	char *p;

	if (s == NULL)
		return NULL;
	
	for (p = s; *p != '\0'; p++)
		*p = tolower(*p);
		
	return s;
}

/* Check if any of the characters in the two strings match. */

int anyof(char *s1, char *s2)
{
	while (*s1)
		if (index(s2, *s1++))
			return 1;
	return 0;
}

/* Check if there are any control characters in there. */

int anycntrls(char *s1)
{
	while (*s1)
		if (iscntrl(*s1++))
			return 1;
	return 0;
}

/* Crash, log the reason */
 
void panic(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vprintf(fmt, args);
	putchar('\n');
	syslog(LOG_NOTICE, fmt, args);
	va_end(args);

	exit(1);
}

/* Report an error returned from a system call. */


void syserr(const char *fmt, ...)
{
	int e = errno;
	va_list args;
	
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	
/*	routine changed by VE3TOK	*/
	printf("Error number: %d\n", e);
	perror("The following error occured: ");

}

