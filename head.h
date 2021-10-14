
/*
 * Structure used to return a break down of a head
 * line (hats off to Bill Joy!)
 */
   
struct headline {
	char	*l_from;	/* The name of the sender */
	char	*l_tty;		/* His tty string (if any) */
	char	*l_date;	/* The entire date string */
};
                           
/* in head.c */
extern char *nextword(char *wp, char *wbuf);
extern char *copyin(char *src, char **space);
extern void parse(char line[], struct headline *hl, char pbuf[]);
extern int ishead(char linebuf[]);
extern int isdate(char date[]);

