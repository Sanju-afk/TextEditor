#include "rawmode.h"
#include "editorfunctions.h"
#include "fileio.h"

static void abAppend(struct abuf* ab, const char* s, int len) {
	char* new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

static void abFree(struct abuf* ab) {
	free(ab->b);
}

static int GetWindowSize(int* rows, int* cols) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	}
	else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

static int EditorCxtoRx(erow* row, int cx) {
	int rx = 0;
	int j;

	for (j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') rx += (TAB_STOP - 1) - (rx % TAB_STOP);
		rx++;
	}
	return rx;
}

static void EditorScroll(struct EditorConfig* E) {
	E->rx = 0;
	//getting correct value of rx 
	if (E->cy < E->numrows) {
		E->rx = EditorCxtoRx(&(E->row[E->cy]), E->cx);
	}

	if (E->cy < E->rowoff) {
		E->rowoff = E->cy;
	}
	if (E->cy >= E->rowoff + E->screenrows) {
		E->rowoff = E->cy - E->screenrows + 1;
	}
	if (E->rx < E->coloff) {
		E->coloff = E->rx;
	}
	if (E->rx >= E->coloff + E->screencols) {
		E->coloff = E->rx - E->screencols + 1;
	}
}

void EditorSetStatusMessage(struct EditorConfig* E,const char* fmt,...) 
{
	va_list	ap;
	va_start(ap, fmt); // now ap contains all args after fmt
	vsnprintf(E->status_message, sizeof(E->status_message), fmt, ap);
	va_end(ap); //to free up memory
	E->status_message_time = time(NULL);
}

static void EditorMessageBar(struct EditorConfig* E, struct abuf* ab)
{
	abAppend(ab, "\x1b[7m", 4); //inverts color
	abAppend(ab, "\x1b[K", 3);
	
	int msglen = strlen(E->status_message);
	if (msglen > E->screencols) msglen = E->screencols;
	if (msglen && ((time(NULL) - E->status_message_time) < 5))abAppend(ab, E->status_message, msglen);
	abAppend(ab, "\x1b[m", 3);
}

static void EditorDrawStatusBar(struct EditorConfig* E,struct abuf* ab) 
{
	abAppend(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	
	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
		E->filename ? E->filename : "[No Name]", E->numrows,
		E->dirty ? "(modified)" : "");

	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",E->cy + 1, E->numrows);

	if (len > E->screencols) len = E->screencols;
	abAppend(ab, status, len);

	while (len < E->screencols) {
		if (E->screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		}
		else {
			abAppend(ab, "*", 1);
			len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);
	abAppend(ab, "\r\n", 2);
}

void InitEditor(struct EditorConfig* E) {
	E->cx = 0;
	E->cy = 0;
	E->row = NULL;
	E->numrows = 0;
	E->rowoff = 0;
	E->coloff = 0;
	E->rx = 0;
	if (GetWindowSize(&(E->screenrows), &(E->screencols)) == -1) die("getWindowSize");
	E->screenrows -= 2;
	E->filename = NULL;
	E->status_message[0] = '\0';
	E->status_message_time = 0;
	E->dirty = 0;
}

static void EditorDrawRows(struct EditorConfig* E, struct abuf* ab) {
	//abAppend(ab, "\x1b[H\x1b[2J", 6);
	for (int i = 0; i < E->screenrows; i++) {
		int filerow = i + E->rowoff;
		if (filerow >= E->numrows) {
			if (E->numrows == 0 && i == E->screenrows / 3) {
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
					"Sanju editor -- version %s", SANJU_VERSION);
				if (welcomelen > E->screencols) welcomelen = E->screencols;
				int padding = (E->screencols - welcomelen) / 2;
				if (padding) {
					abAppend(ab, "~", 1);
					padding--;
				}
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
			}
			else {
				abAppend(ab, "~", 1);
			}
		}
		else {
			int rowlen = E->row[filerow].render_size - E->coloff;
			if (rowlen < 0) rowlen = 0;
			if (rowlen > E->screencols) rowlen = E->screencols;
			abAppend(ab, &(E->row[filerow].render[E->coloff]), rowlen);
		}

		abAppend(ab, "\x1b[K", 3);
		abAppend(ab, "\r\n", 2); 
	}
}

void EditorRefreshScreen(struct EditorConfig* E) 
{	
	write(STDIN_FILENO, "\x1b[40m", 5); //background 
	write(STDIN_FILENO, "\x1b[36m", 5); //font 

	EditorScroll(E);
	struct abuf ab = ABUF_INIT;

	//Update screen size if needed
	int rows, cols;
	if (GetWindowSize(&rows, &cols) != -1) {
		if (rows != E->screenrows || cols != E->screencols) {
			//to include status bar and message bar
			E->screenrows = rows - 2;
			E->screencols = cols;
		}
	}

	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	EditorDrawRows(E, &ab);
	EditorDrawStatusBar(E, &ab);
	EditorMessageBar(E, &ab);
	

	//to move cursor
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E->cy - E->rowoff + 1, E->rx - E->coloff + 1);
	abAppend(&ab, buf, strlen(buf));	
	
	abAppend(&ab, "\x1b[?25h", 6);
	//final writing of whole buffer
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

static int EditorReadKey() 
{
	int nread;
	char c;;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno == EAGAIN) die("Read error");
	}
	if (c == '\x1b') {
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
					case '1': return HOME_KEY;
					case '4': return END_KEY;
					case '3': return DELETE_KEY;	
					case '5': return PAGE_UP;
					case '6': return PAGE_DOWN;
					case '7': return HOME_KEY;
					case '8': return END_KEY;
					}
				}
			}
			else {
				switch (seq[1]) {
				case 'A': return ARROW_UP;
				case 'B': return ARROW_DOWN;
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
				}
			}
		}
		else if (seq[0] == 'O') {
			switch (seq[1]) {
			case 'H': return HOME_KEY;
			case 'F': return END_KEY;
			}
		}
		return '\x1b';
	}
	else {
		return c;
	}	
}

static void EditorMoveCursor(struct EditorConfig* E, int key) {
	erow* row = (E->cy >= E->numrows) ? NULL : &(E->row[E->cy]);

	switch (key) {
	case ARROW_LEFT:
		if (E->cx != 0)E->cx--;
		//to go to prev line end
		else if (E->cy > 0) {
			E->cy--;
			E->cx = E->row[E->cy].size;
		}
		break;
	case ARROW_RIGHT:
		if (row && E->cx < row->size)E->cx++;
		//to go to start of next line
		else if ((E->cy < E->numrows) && E->cx == row->size && row) {
			E->cy++;
			E->cx = 0;
		}
		break;
	case ARROW_UP:
		if (E->cy != 0)E->cy--;
		break;
	case ARROW_DOWN:
		if (E->cy < E->numrows)E->cy++;
		break;
	}

	row = E->cy > E->numrows ? NULL : &(E->row[E->cy]);
	//while switching from a longer line to a shorter one, cursor should get adjusted
	int rowlen = row ? row->size : 0;
	if (E->cx > rowlen) E->cx = rowlen;
}

void EditorProcessKeyPress(struct EditorConfig* E)
{
	int c = EditorReadKey();
	static int quit = SANJU_QUIT;

	switch (c) {
	//quit
	case CTRL_KEY('q'):
		if (E->dirty && quit > 0) {
			EditorSetStatusMessage(E,"WARNING!!The file has unsaved changes, press CTR-Q %d times to quit", quit);
			quit--;
			return;
		}
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(0);
		break;


	//save
	case CTRL_KEY('s'):
		EditorSave(E);
		break;


	//special charecters
		//Enter key
	case '\r':
		EditorInsertNewLine(E);
		break;
	case DELETE_KEY:
		//del is equivalent to moving cursor right and then backspace
		EditorMoveCursor(E, ARROW_RIGHT);
		EditorDelChar(E);
		break;
	case BACKSPACE:
	case CTRL_KEY('h'):
		EditorDelChar(E);
		break;


	//home and end keys
	case HOME_KEY:
		E->cx = 0;
		break;
	case END_KEY:
		if (E->cy < E->numrows) {
			E->cx = E->row[E->cy].size;
		}
		break;


	//page down and ups
	case PAGE_UP:
		E->cy = E->rowoff;
		break;
	case PAGE_DOWN:
		E->cy = E->rowoff + E->screenrows - 1;
		if (E->cy > E->numrows) { E->cy = E->numrows; }
		break;


	//arrow keys
	case ARROW_UP:
	case ARROW_DOWN:
	case ARROW_LEFT:
	case ARROW_RIGHT:
		EditorMoveCursor(E,c);
		break;

	//ctrl+l is used as refresh, we ignore esc and this char
	case '\x1b':
	case CTRL_KEY('l'):
		break;



	//other charecters are written down
	default:
		EditorInsertChar(E, c);
		break;
	}
	//resetting quit value if any other key is pressed
	quit = SANJU_QUIT;
}