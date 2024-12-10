#pragma once
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define SANJU_QUIT 3
#define CTRL_KEY(k) (k & 0x1f)
#define SANJU_VERSION "0.0.2"
#define TAB_STOP 8
#include<termios.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<ctype.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<stdarg.h>
#include <fcntl.h>

//to store a row of text
typedef struct erow {
	int size;
	char* chars;
	//to manage non printable charecters
	char* render;
	int render_size;

}erow;

//buffer to replace write operations
struct abuf {
	char* b;
	int len;
};

#define ABUF_INIT {NULL, 0}

struct EditorConfig {
	int cx, cy; //cursor cordinates
	int rx; //to account for non printable chars like tab
	int screenrows;
	int screencols;
	int rowoff; //rowoffset for scroolling
	int coloff;
	int numrows; //number of rows in file
	erow* row;
	char* filename;
	char status_message[80];
	time_t status_message_time;
	int dirty;
	struct termios orig_termios; //terminal attributes for raw mode
};

enum editorKey {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT ,
	ARROW_UP ,
	ARROW_DOWN,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	DELETE_KEY
};

extern struct EditorConfig E;

void DisableRawMode(struct EditorConfig* E);

void EnableRawMode(struct EditorConfig* E);

void die(const char* s);

void Cleanup();