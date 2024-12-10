#pragma once
#include "rawmode.h"

void EditorProcessKeyPress(struct EditorConfig* E);	

void EditorRefreshScreen(struct EditorConfig* E);

//int GetCursorPosition(int *rows, int *cols);

void InitEditor(struct EditorConfig* E);

void EditorSetStatusMessage(struct EditorConfig* E, const char* fmt, ...);