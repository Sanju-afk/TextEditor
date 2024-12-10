#include<termios.h>
#include<unistd.h>
#include<errno.h>
#include<ctype.h>
#include<stdlib.h>
#include<stdio.h>

#include "rawmode.h"

//exit function
void die(const char* s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	perror(s);
	exit(1);
}

//we are changing the terminal bit attributes
//turning echo back on,ie turn echo back on while exiting
void DisableRawMode(struct EditorConfig* E) {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &(E->orig_termios)) == -1) {
		die("tcsetattr error");
	}
}

void Cleanup(void) {
	DisableRawMode(&E); // Call the function with arguments
}

//turning off echo on starting our prgrm
void EnableRawMode(struct EditorConfig* E) {
	if (tcgetattr(STDIN_FILENO, &(E->orig_termios)) == -1)die("tcgetattr error");

	atexit(Cleanup);
	//struct to store attributes for raw mode
	struct termios raw = E->orig_termios;

	//flipping local flag bits for raw mode
	//we flip bits using negation and logical AND
	raw.c_cflag &= ~(CS8);
	raw.c_oflag &= ~(OPOST);
	raw.c_iflag &= ~(IXON | ICRNL | INPCK | ISTRIP | BRKINT);
	raw.c_lflag &= ~(ECHO | IEXTEN | ICANON | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr error");
}

