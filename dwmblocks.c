#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<assert.h>
#ifndef NO_X
#include<X11/Xlib.h>
#endif
#ifdef __OpenBSD__
#define SIGPLUS			SIGUSR1+1
#define SIGMINUS		SIGUSR1-1
#else
#define SIGPLUS			SIGRTMIN
#define SIGMINUS		SIGRTMIN
#endif
#define LENGTH(X)               (sizeof(X) / sizeof (X[0]))
#define CMDLENGTH		50
#define MIN( a, b ) ( ( a < b) ? a : b )
#define STATUSLENGTH (LENGTH(blocks) * CMDLENGTH + 1)

typedef struct {
	char* icon;
	char* command;
	unsigned int interval;
	unsigned int signal;
} Block;
#ifndef __OpenBSD__
void dummysighandler(int num);
#endif
void sighandler(int num);
void getcmds(int time);
void getsigcmds(unsigned int signal);
void setupsignals();
void sighandler(int signum);
int getstatus(char *str);
void statusloop();
void termhandler(int);
void pstdout();
#ifndef NO_X
void setroot();
static void (*writestatus) () = setroot;
static int setupX();
static Display *dpy;
static int screen;
static Window root;
#else
static void (*writestatus) () = pstdout;
#endif


#include "blocks.h"

static char statusbar[LENGTH(blocks)][CMDLENGTH] = {0};
static unsigned int statusbarlen[LENGTH(blocks)];
static char statusstr[STATUSLENGTH];
static int statusContinue = 1;
static int statusChanged = 0;
static int returnStatus = 0;

//return ptr to nul terminator in dst
char *xstpcpyLen(char *dst, const char *src, size_t n)
{
	dst = (char *)memcpy(dst, src, n) + n;
	*dst = '\0';
	return dst;
}

//opens process *cmd and stores output in *output
char *getcmd(const Block *block, char *output, unsigned int outputOldLen)
{
	//make sure status is same until output is ready
	char tempstatus[CMDLENGTH];
	char *endp = tempstatus;
	endp = xstpcpyLen(endp, block->icon, strlen(block->icon));
	FILE *cmdf = popen(block->command, "r");
	if (!cmdf)
		return output + outputOldLen;
	unsigned int readLen = fread(endp, 1, CMDLENGTH-(endp-tempstatus)-delimLen, cmdf);
	pclose(cmdf);
	tempstatus[readLen] = '\0';
	//if block and command output are both not empty
	if (readLen) {
		//chop off newline
		char *nl = memchr(tempstatus, '\n', readLen);
		if (nl) {
			nl[0] = '\0';
			endp = nl;
		}
		endp = xstpcpyLen(endp, delim, MIN(delimLen, CMDLENGTH-(endp-tempstatus)));
	}
	//mark if there is a change and copy
	if (outputOldLen != endp-tempstatus || memcmp(tempstatus, output, outputOldLen)) {
		statusChanged = 1;
		endp = xstpcpyLen(output, tempstatus, endp-tempstatus);
		return endp;
	} else {
		return output + outputOldLen;
	}
}

void getcmds(int time)
{
	const Block* current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if ((current->interval != 0 && time % current->interval == 0) || time == -1) {
			//cache strlen
			statusbarlen[i] = getcmd(current,statusbar[i],statusbarlen[i]) - statusbar[i];
		}
	}
}

void getsigcmds(unsigned int signal)
{
	const Block *current;
	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		current = blocks + i;
		if (current->signal == signal)
			//cache strlen
			statusbarlen[i] = getcmd(current,statusbar[i],statusbarlen[i]) - statusbar[i];
	}
}

void setupsignals()
{
#ifndef __OpenBSD__
	    /* initialize all real time signals with dummy handler */
    for (int i = SIGRTMIN; i <= SIGRTMAX; i++)
        signal(i, dummysighandler);
#endif

	for (unsigned int i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal > 0)
			signal(SIGMINUS+blocks[i].signal, sighandler);
	}

}

int getstatus(char *str)
{
	char *p = str;
	for (unsigned int i = 0; i < LENGTH(blocks); i++)
		p = xstpcpyLen(p, statusbar[i], statusbarlen[i]);
	if (p != str) {
		p -= delimLen;
		*p = '\0';
	}
	return 1;
}

#ifndef NO_X
void setroot()
{
	getstatus(statusstr);
	XStoreName(dpy, root, statusstr);
	XFlush(dpy);
}

int setupX()
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "dwmblocks: Failed to open display\n");
		return 0;
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	return 1;
}
#endif

void pstdout()
{
	getstatus(statusstr);
	printf("%s\n",statusstr);
	fflush(stdout);
}


void statusloop()
{
	setupsignals();
	int i = 0;
	getcmds(-1);
	while (1) {
		getcmds(i++);
		if (statusChanged)//Only write out if text has changed.
			writestatus();
		statusChanged = 0;
		if (!statusContinue)
			break;
		sleep(1.0);
	}
}

#ifndef __OpenBSD__
/* this signal handler should do nothing */
void dummysighandler(int signum)
{
    return;
}
#endif

void sighandler(int signum)
{
	getsigcmds(signum-SIGPLUS);
	writestatus();
}

void termhandler(int unused)
{
	statusContinue = 0;
}

int main(int argc, char** argv)
{
	for (int i = 0; i < argc; i++) {//Handle command line arguments
		if (!strcmp("-d",argv[i]))
			strncpy(delim, argv[++i], delimLen);
		else if (!strcmp("-p",argv[i]))
			writestatus = pstdout;
	}
#ifndef NO_X
	if (!setupX())
		return 1;
#endif
	delimLen = MIN(delimLen, strlen(delim));
	delim[delimLen] = '\0';
	signal(SIGTERM, termhandler);
	signal(SIGINT, termhandler);
	statusloop();
#ifndef NO_X
	XCloseDisplay(dpy);
#endif
	return 0;
}
