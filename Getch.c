
/*
	Version: 19/08/2014
	contact: danilosantos_23@hotmail.com
	elaborated based on discussions in stack overflow
		*Linux version 100% operational, termios isn't an alien pre-requisite
		*Windows version, I need to check on home.
*/

/*
	Explanation on the timers: The termios library works with the smallest resolution of 0.1s and 1 byte total size. The minimum number of time allowed is 100ms, so to keep cross-platform all my functions will have this limitation! I tried my best to make both platforms experiment the same behaviour, but Windows isn't as precise as Linux. If you ask for a 3.4s in windows, I cannot guarantee it will be 3.4s [though it should be close]

	Let me be clear:
		value of timeout	behaviour
		0			no timeout mode!
		1-25500			timeout = ceil (timeout/100)
		25500+			timeout = 25500 ms		(capped at 25.5s, the )
*/

#define DEFAULT_TIMER	2000	/*A default timer for the input, 2s is enough even for slow typers to press one key.*/

/*IF WE ARE IN A LINUX ENVIRONMENT, USE THE TERMIOS STUFF*/
#ifdef __unix__
	#include <termios.h>
	#define NO_TIMEOUT	0

/*********************************************************************************************************************************************/
/*FUNCTION DECLARATIONS*/
/*"Public", Intended to be called from outside this file*/
	char  getch (void);				/* Read 1 character without echo */
	char tgetch (int timeout);			/* Read 1 character without echo, returns EOF if it timeouts */
	char  getche(void);				/* Read 1 character with echo */
	char tgetche(int timeout);			/* Read 1 character with echo, returns EOF if it timeouts*/

/*"Private", not intended to be called outside this file*/
	char getch_(int echo, int timeout);

/*********************************************************************************************************************************************/
/*FUNCTION IMPLEMENTATION*/

/* Read 1 character
	-> echo defines echo mode [switching from getch() and getche()]
	-> timer defines the timeout time, 0 means no timeout (usual getch/getche) and 1-255 means the timeout is set to 0.1 to 25.5 seconds, meaning that if the user doesn't give input for a time longer than this "timer", it returns EOF
*/
char getch_(int echo, int timeout) {
	char ch;

	/*Adjusting the timeout from ms to ds and setting up the caps*/
	if (timeout != NO_TIMEOUT) {
		if (timeout > 25500)						/*Capping it it goes higher than 25.5s*/
			timeout = 255;
		else								/*Ceiling*/
			timeout = ((timeout % 100) > 0) + timeout/100;
	}

	/*BACKING UP THE TERMINAL IO SETTINGS*/
	static struct termios old_console, new_console;
	tcgetattr(0, &old_console);			/* grab old terminal i/o settings */
	new_console = old_console;			/* copy the old settings */

	/*ALTERING THE TERMINAL SETTINGS*/
	new_console.c_lflag &= ~ICANON;			/* disable buffered i/o */
	new_console.c_lflag &= echo ? ECHO : ~ECHO;	/* set echo mode to the user's input*/
	new_console.c_cc[VMIN]=0;			/* it's allowed to return empty-handed*/
	new_console.c_cc[VTIME]=timeout;			/* if the user doesn't give input for longer than "timeout", returns EOF*/
	tcsetattr(0, TCSANOW, &new_console);		/* use these newly defined i/o settings now */

	/*GRABBING INPUT*/
	ch = getchar();

	/*RESTORING THE SETTINGS FROM THE BACKUP*/
	tcsetattr(0, TCSANOW, &old_console);
	return ch;
}

char  getch (void)		{ return getch_(0, NO_TIMEOUT);}
char tgetch (int timeout)	{ return getch_(0, timeout);}
char  getche(void)		{ return getch_(1, NO_TIMEOUT);}
char tgetche(int timeout)	{ return getch_(1, timeout);}

/*IF WE ARE IN A WINDOWS ENVIRONMENT, LET'S USE CONIO AND JUST IMPLEMENT THE TIMED FUNCTIONS*/
#elif defined(_WIN32) || defined(WIN32)

#include <conio.h>
#include <time.h>

/*********************************************************************************************************************************************/
/*FUNCTION DECLARATIONS*/

/*"Public", intended to be called from other file*/
char tgetch (int timeout);						/*Reads 1 character, no echo and returns EOF if it timeouts*/
char tgetche (int timeout);						/*Reads 1 character, with echo and returns EOF if it timeouts*/

/*"Private", not intended to be called from outside this file*/
char tgetch_ (int timeout, int echo);

/*********************************************************************************************************************************************/
/*FUNCTION IMPLEMENTATION*/
char tgetch (int timeout) {
	time_t start, now;
	char ch;

	/*Adding a bunch of cycles before giving up, should take 0.1-0.9s depending on timeout*/
	int ds = 1000*(timeout%100);

	/*Adjusting the timeout from ms to ds and setting up the caps*/
	if (timeout != NO_TIMEOUT) {
		if (timeout > 25500)						/*Capping it it goes higher than 25.5s*/
			timeout = 25;
		else
			timeout = timeout/1000;
	}

	time(&start);
	while (!_kbhit()) {
		time(&now);


		if (now-start > timeout && ds>0)		/*When the seconds match, start reducing the extra iterations*/
			timeout--;
		else if (now-start > timeout && ds == 0)	/*When both run out, return EOF*/
			return EOF;
	}

	ch = getchar();

	if (echo)
		putchar(ch);
	return ch;
}

char tgetch (int timeout)		{return tgetch_ (timeout, 0);}
char tgetche (int timeout)		{return tgetch_ (timeout, 1);}

#endif

