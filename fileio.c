#include "rawmode.h"
#include "viewerfunctions.h"

//update render
void EditorUpdateRow(erow* row) {
	int tabs = 0;
	int j;

	for (j = 0;j < row->size;j++) {
		if (row->chars[j] == '\t') tabs++;
	}

	free(row->render);
	row->render = malloc(row->size + tabs*(TAB_STOP - 1) + 1);

	int idx = 0;
	for (j = 0;j < row->size;j++) {
		if (row->chars[j] == '\t') {
			//in chars tab is one space, we need to convert it to 8 spaces	
			row->render[idx++] = ' ';
			while (idx % TAB_STOP != 0) row->render[idx++] = ' ';
		}
		else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[j] = '\0';
	row->render_size = j;
}

//add a row
void EditorInsertRow(struct EditorConfig* E, int at,char* s, size_t len)
{
	if (at<0 || at>E->numrows) return;
	//allocate memory first
	E->row = realloc(E->row, sizeof(erow) * (E->numrows + 1 ));

	memmove(&E->row[at + 1], &E->row[at], sizeof(erow) * (E->numrows - at));

	E->row[at].size = len;
	E->row[at].chars = malloc(len + 1);
	memcpy(E->row[at].chars, s, len);
	E->row[at].chars[len] = '\0';
	
	E->row[at].render = NULL;
	E->row[at].render_size = 0;
	EditorUpdateRow(&(E->row[at]));

	E->numrows++;
	E->dirty++;
}	

//open and read file
void EditorOpen(struct EditorConfig *E, char* filename) 
{	
	FILE* fp;
	free(E->filename);
	E->filename = strdup(filename);
	if (access(filename, F_OK) == 0) {
		fp = fopen(filename, "r+");
	}
	else {
		fp = fopen(filename, "w");
	}
	if (!fp) die("file opening error");
	char* line = NULL;
	size_t bufferlen = 0; //buffer to store lines read
	ssize_t linelen;

	while ((linelen = getline(&line, &bufferlen, fp)) != -1) 
	{
		while (linelen > 0 && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
		{
			linelen--; //to exclude all newline and carriages
		}
		EditorInsertRow(E,E->numrows,line, linelen);//add a line
	}	
	free(line); //release memory alloted by getline
	fclose(fp);
	E->dirty = 0;
	// Initialize to the top of the file 
	E->cx = 0; 
	E->cy = 0; 
	E->rowoff = 0;
}

//to convert our rows to a char pointer buffer
static char* EditorRowsToString(struct EditorConfig* E, int* buflen)
{
	int totlen = 0;
	int i;
	for (i = 0; i < E->numrows;i++) {
		totlen += E->row[i].size + 1;
	}
	*buflen = totlen;

	char* buf = malloc(totlen);
	char* p = buf;

	for (i = 0; i < E->numrows; i++) {
		memcpy(p, E->row[i].chars, E->row[i].size);
		p += E->row[i].size; //shift memory to end of line
		*p = '\n';
		p++; //shift memory to end of newline sequence, ie start of next lines memory	
	}

	return buf;
}

//to save to disk, mapped to ctr-S
void EditorSave(struct EditorConfig* E)
{
	if (E->filename == NULL) return;
	
	int len = 0;
	char* buf = EditorRowsToString(E,&len);

	//opens an existing file, or creates a new one if doesnt exists
	int fd = open(E->filename, O_RDWR | O_CREAT, 0644);
	
	//error handling
	if (fd != -1) {
		if (ftruncate(fd, len) != -1) { //try to set the file size = len
			if (write(fd, buf, len) == len) { //try to write buf of size len to file
				close(fd);
				free(buf);
				E->dirty = 0;
				EditorSetStatusMessage(E,"%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	EditorSetStatusMessage(E,"Cant Save! I/O error : %s", strerror(errno));
}