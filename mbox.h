/*
 * flag bits.
 */

#define MUSED		(1<<0)		/* entry is used, but this bit isn't */
#define MDELETED	(1<<1)		/* entry has been deleted */
#define MSAVED		(1<<2)		/* entry has been saved */
#define MTOUCH		(1<<3)		/* entry has been noticed */
#define MPRESERVE	(1<<4)		/* keep entry in sys mailbox */
#define MMARK		(1<<5)		/* message is marked! */
#define MODIFY		(1<<6)		/* message has been modified */
#define MNEW		(1<<7)		/* message has never been seen */
#define MREAD		(1<<8)		/* message has been read sometime. */
#define MSTATUS		(1<<9)		/* message status has changed */

/*
 * Structure used in the message list
 */

struct message {
	short	m_flag;			/* flags, see below */
	long	m_offset;		/* offset of message */
	long	m_size;			/* Bytes in the message */
	short	m_lines;		/* Lines in the message */
	char	*receipt;		/* Receipt request */
	char	*notify;		/* Delivery notify */
	char	*from;			/* From */
	char	*date;			/* Date string */
	char	*subj;			/* Subject line */
	char	*id;			/* Message ID */
	char	*whoti;			/* Who to send a fax to */
	char	*phone;			/* Phone to fax */
	char	*note;			/* Note for fax cover */
};

extern FILE *mbox;		/* Temporary mailbox in /tmp */
extern struct message *message;	/* Message list */
extern struct message *dot;	/* Current message pointer */
extern int messages;		/* Amount of messages in list */
extern int current;		/* Current message index */
extern int newm;		/* New messages */

extern long sysboxlen;		/* System mailbox length at start */

extern int getmail(void);
extern int readmesg(int msg, int verbose);
