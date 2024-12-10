//myheaders
#include "viewerfunctions.h"
#include "fileio.h"

//this instance will be shared among all files
struct EditorConfig E;

int main(int argc, char* argv[])
{	
	EnableRawMode(&E);
	InitEditor(&E);
	
	if (argc > 1) {
		EditorOpen(&E, argv[1]);
	}
	
	EditorSetStatusMessage(&E,"HELP: CTR-Q = quit | CTR-S = save");

	while (1) {
		EditorRefreshScreen(&E);
		EditorProcessKeyPress(&E);
	}

	return 0;
}	