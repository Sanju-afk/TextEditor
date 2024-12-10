#pragma once
#include "rawmode.h"

void EditorInsertRow(struct EditorConfig* E,int at, char* s, size_t len);

void EditorOpen(struct EditorConfig* E, char* filename);

void EditorUpdateRow(erow* row);

void EditorSave(struct EditorConfig* E);