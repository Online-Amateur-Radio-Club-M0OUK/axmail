
/* head.c - parse mail headers.
   Grabbed from mailx 5.3b, reformatted, hacked and cleaned
   to compile without warnings by Heikki Hannikainen <hessu@pspt.fi> */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * Mail -- a mail program
 *
 * Routines for processing and detecting headlines.
 */

#include <stdio.h>
#include <ctype.h>

#include "head.h"
#include "defines.h"

/*
 * See if the passed line buffer is a mail header.
 * Return true if yes.  Note the extreme pains to
 * accomodate all funny formats.
 */

int ishead(char linebuf[])
{
	char *cp;
	struct headline hl;
	char parbuf[LINESIZE];

	cp = linebuf;
	if (*cp++ != 'F' || *cp++ != 'r' || *cp++ != 'o' || *cp++ != 'm' ||
	    *cp++ != ' ')
		return 0;
	parse(linebuf, &hl, parbuf);
	if (hl.l_from == NULL || hl.l_date == NULL) {
		/* No from or date field */
		return 0;
	}
	if (!isdate(hl.l_date)) {
		/* Date field not legal date */
		return 0;
	}
	/*
	 * I guess we got it!
	 */
	return 1;
}

/*
 * Split a headline into its useful components.
 * Copy the line into dynamic string space, then set
 * pointers into the copied line in the passed headline
 * structure.  Actually, it scans.
 */

void parse(char line[], struct headline *hl, char pbuf[])
{
	char *cp, *sp;
	char word[LINESIZE];

	hl->l_from = NULL;
	hl->l_tty = NULL;
	hl->l_date = NULL;
	cp = line;
	sp = pbuf;
	/*
	 * Skip over "From" first.
	 */
	cp = nextword(cp, word);
	cp = nextword(cp, word);
	if (*word)
		hl->l_from = copyin(word, &sp);
	if (cp != NULL && cp[0] == 't' && cp[1] == 't' && cp[2] == 'y') {
		cp = nextword(cp, word);
		hl->l_tty = copyin(word, &sp);
	}
	if (cp != NULL)
		hl->l_date = copyin(cp, &sp);
}

/*
 * Copy the string on the left into the string on the right
 * and bump the right (reference) string pointer by the length.
 * Thus, dynamically allocate space in the right string, copying
 * the left string into it.
 */

char *copyin(char *src, char **space)
{
	char *cp, *top;

	top = cp = *space;
	while ((*cp++ = *src++))
		;
	*space = cp;
	return top;
}

#define	L	1		/* A lower case char */
#define	S	2		/* A space */
#define	D	3		/* A digit */
#define	O	4		/* An optional digit or space */
#define	C	5		/* A colon */
#define	N	6		/* A new line */
#define U	7		/* An upper case char */

char ctypes[] = { U,L,L,S,U,L,L,S,O,D,S,D,D,C,D,D,C,D,D,S,D,D,D,D,0 };
char tmztypes[] = { U,L,L,S,U,L,L,S,O,D,S,D,D,C,D,D,C,D,D,S,U,U,U,S,D,D,D,D,0 };

/*
 * Match the given string (cp) against the given template (tp).
 * Return 1 if they match, 0 if they don't
 */

int cmatch(char *cp, char *tp)
{

	while (*cp && *tp)
		switch (*tp++) {
		case L:
			if (!islower(*cp++))
				return 0;
			break;
		case U:
			if (!isupper(*cp++))
				return 0;
			break;
		case S:
			if (*cp++ != ' ')
				return 0;
			break;
		case D:
			if (!isdigit(*cp++))
				return 0;
			break;
		case O:
			if (*cp != ' ' && !isdigit(*cp))
				return 0;
			cp++;
			break;
		case C:
			if (*cp++ != ':')
				return 0;
			break;
		case N:
			if (*cp++ != '\n')
				return 0;
			break;
		}
	if (*cp || *tp)
		return 0;
	return 1;
}

/*
 * Test to see if the passed string is a ctime(3) generated
 * date string as documented in the manual.  The template
 * below is used as the criterion of correctness.
 * Also, we check for a possible trailing time zone using
 * the auxtype template.
 */

int isdate(char date[])
{

	if (cmatch(date, ctypes))
		return 1;
	return cmatch(date, tmztypes);
}

/*
 * Collect a liberal (space, tab delimited) word into the word buffer
 * passed.  Also, return a pointer to the next word following that,
 * or NULL if none follow.
 */

char *nextword(char *wp, char *wbuf)
{
	char c;

	if (wp == NULL) {
		*wbuf = 0;
		return NULL;
	}
	while ((c = *wp++) && c != ' ' && c != '\t') {
		*wbuf++ = c;
		if (c == '"') {
 			while ((c = *wp++) && c != '"')
 				*wbuf++ = c;
 			if (c == '"')
 				*wbuf++ = c;
			else
				wp--;
 		}
	}
	*wbuf = '\0';
	for (; c == ' ' || c == '\t'; c = *wp++)
		;
	if (c == 0)
		return NULL;
	return (wp - 1);
}
