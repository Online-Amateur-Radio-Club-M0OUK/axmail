
/* adduser.c - create an user account (yecch) */
 
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <crypt.h>
#include <string.h>

#include "adduser.h"
#include "utils.h"
#include "config.h"
#include "defines.h"

/* Ask for a new password, and keep it sane */

void get_passwd(char *newuser, char *passw)
{
	printf("Please email %s with your callsign and desired password.\n", sysopmail);

	char passi[12];

repass: /* Get password */
	getstr(passi, 10, "Enter password for the new account (6-8 characters, ? for help): ");

	if (!strcmp(passi, "?")) {
		printf("Your password must be 6 to 8 characters long, and it may not be the same as\n");
		printf("your login name (in this case, your callsign).\n");
		goto repass;
	}
	if ((strlen(passi) > 8) || (strlen(passi) < 6)) {
		printf("Password must be 6-8 characters long.\n");
		goto repass;
	}
	if (!strcasecmp(passi, newuser)) {
		printf("Password may not be your login name (callsign).\n");
		goto repass;
	}

	/* Crypt password */
	strcpy(passw, crypt(passi, "ax")); /* Okay, salt _should_ be random... */
}

/* add a new user to /etc/passwd and do some init */

int new_user(char *newuser)
{
	struct passwd pw, *pwp;
	uid_t uid;
	FILE *fp;
	char homedir[256], userdir[256];
	char buf[4096];
	char subdir[4];
	char passw[20];
	char str[LINESIZE + 1];
	int cnt = 0;
	unsigned char *p, *q;
	struct stat fst;
	int fd_a, fd_b, fd_l;
	
	/* Build path for home directory */

	strncpy(subdir, newuser, 3);
	subdir[3] = '\0';
	sprintf(homedir, "%s/%s", def_homedir, newuser);
	strcpy(userdir, homedir);

	getname(fullname);

	if (strlen(fullname) == 0) {
		printf("Okay, using your callsign as your name. You may change it later.\n");
		strcpy(fullname, newuser);
	}

	/* Get password */

	switch (login_allowed)	{
		case 0:	strcpy(passw, "*");
			break;
		case 1: get_passwd(newuser, passw);
			break;
		case 2: strcpy(passw, ""); /* Yuck! */
			break;
	}
	
	/* Lock */
	fd_l = open(LOCK_AXMAIL_FILE, O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	flock(fd_l, LOCK_EX);

retry:
	/* Find first free UID */
	cnt++;

	for (uid = first_uid; uid < 65535; uid++)
	{
		pwp = getpwuid(uid);
		if (pwp == NULL)
			break;
	}

	if (uid >= 65535 || uid < first_uid)
		return -1;

	/* build directories for home */

	p = homedir;
		
	while (*p == '/') p++;

	chdir("/");

	while(p)
	{
		q = strchr(p, '/');
		if (q)
		{
			*q = '\0';
			q++;
			while (*q == '/') q++;
			if (*q == 0) q = NULL;
		}

		if (stat(p, &fst) < 0)
		{
			if (errno == ENOENT)
			{
				mkdir(p, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);

				if (q == NULL)
				{
					chown(p, uid, user_gid);
					chmod(p, S_IRUSR|S_IWUSR|S_IXUSR);
				} 
			}
			else
				return -1;
		}
		
		if (chdir(p) < 0)
			return -1;
		p = q;
	}

	/* Add the user now */

	fp = fopen(PASSWDFILE, "a+");
	if (fp == NULL)
		return -1;
	
	pw.pw_name   = newuser;
	pw.pw_passwd = passw;
	pw.pw_uid    = uid;
	pw.pw_gid    = user_gid;
	pw.pw_gecos  = fullname;
	pw.pw_dir    = userdir;
	pw.pw_shell  = def_shell;

	if ((getpwuid(uid) != NULL) && (cnt <= 10)) goto retry; /* oops?! */

	if ((putpwent(&pw, fp) < 0) || (cnt > 10))
		return -1;

	flock(fd_l, LOCK_UN);
	fclose(fp);

	/* Copy ax25.profile */
	
	fd_a = open(CONF_AXMAIL_PROF_FILE, O_RDONLY);
	
	if (fd_a > 0)
	{
		fd_b = open(USERPROFILE, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IXUSR);

		if (fd_b < 0)
			return -1;
		
		while ( (cnt = read(fd_a, &buf, sizeof(buf))) > 0 )
			write(fd_b, &buf, cnt);
		close(fd_b);
		close(fd_a);
		chown(USERPROFILE, uid, user_gid);
	
	}
/* Be nice and send the new user a welcome message :) */

	sprintf(str, "%s -oem %s@%s < %s", BIN_AXMAIL_SENDMAIL, newuser, hostname, WELCOME);
                system(str);
	return 0;
}

