#include "rawmode.h"
#include "fileio.h"
#include "viewerfunctions.h"

//deletes a char at a position
static void EditorRowDelChar(struct EditorConfig* E, erow* row, int at) 
{
	if (at<0 || at > row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	EditorUpdateRow(row);
	E->dirty++;
}

//inserts a char at a position
static void EditorRowInsertChar(struct EditorConfig* E,erow* row, int at, int c)
{
	if (at<0 || at>row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);

	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	EditorUpdateRow(row);
	E->dirty++;
}

static void EditorFreeRow(erow* row)
{
	free(row->chars);
	free(row->render);
}

static void EditorDelRow(struct EditorConfig* E, int at)
{
	if (at < 0 || at >= E->numrows) return;

	EditorFreeRow(&E->row[at]);
	memmove(&E->row[at], &E->row[at + 1], sizeof(erow) * (E->numrows - at - 1));
	E->numrows--;
	E->dirty++;
}

//appends a string to a row
static void EditorRowAppendString(struct EditorConfig* E, erow* row, char* s, size_t len)
{
	row->chars = realloc(row->chars, row->size + len + 1);
	memcpy(&row->chars[row->size], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	EditorUpdateRow(row); //update render using chars
	E->dirty++;
}

//to handle enter
void EditorInsertNewLine(struct EditorConfig* E)
{
	//insert a blank line before current line
	if (E->cx == 0) {
		EditorInsertRow(E, E->cy, "",0);
	}
	else {
		erow* row = &E->row[E->cy];
		EditorInsertRow(E, E->cy + 1, &row->chars[E->cx], row->size - E->cx);
		row = &E->row[E->cy];
		row->size = E->cx;
		row->chars[row->size] = '\0';
		EditorUpdateRow(row); //update render
	}
	//go to start of next line
	E->cy++;	
	E->cx = 0;	
}

void EditorInsertChar(struct EditorConfig* E,int c)
{
	if (E->cy == E->numrows) {
		EditorInsertRow(E,E->numrows,"", 0); //add an empty line/row at end
	}
	EditorRowInsertChar(E,&(E->row[E->cy]), E->cx, c);
	E->cx++;
}	

//gets mapped to backspace and delete and ctrl-h
void EditorDelChar(struct EditorConfig* E)
{
	if (E->cy == E->numrows) return; //past the content theres nothing to delete
	if (E->cx == 0 && E->cy == 0) return; //beggining of file

	erow* row = &(E->row[E->cy]);
	if (E->cx > 0) {
		EditorRowDelChar(E, row, E->cx - 1);
		E->cx--;
	}
	//cursor is at beggining of file
	// we need to append current line at end of prev line
	else {
		E->cx = E->row[E->cy - 1].size; //at end of prev line
		EditorRowAppendString(E, &E->row[E->cy - 1], row->chars, row->size);
		EditorDelRow(E, E->cy); //delete current row
		E->cy--;
	}
}


