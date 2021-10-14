
extern void prompt(void);

extern void getname(char *uname);
extern void getstr(char *str, int len, char *prompt);

extern char *strupr(char *s);
extern char *strlwr(char *s);
extern int anyof(char *s1, char *s2);
extern int anycntrls(char *s1);

extern void panic(const char *fmt, ...);
extern void syserr(const char *fmt, ...);
/* n1uro */
char callsign[20];
