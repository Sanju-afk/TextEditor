#pragma once
#include "rawmode.h"

void EditorInsertChar(struct EditorConfig* E, int c);

void EditorDelChar(struct EditorConfig* E);

void EditorInsertNewLine(struct EditorConfig* E);