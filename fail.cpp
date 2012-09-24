#include "types.h"
#include "x11.h"
#include "fail.h"
#include "io.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/*		basic fallback		*/

void fail(const char* msg) {
	fprintf(stderr,"%s\n", msg);
	clean_up();
	exit(1);
}

void sig_exit(int param) {
	clean_up();
	signal(SIGINT,SIG_DFL);
	raise(SIGINT);
	exit(0);
}

int handle_error(Display* dsp, XErrorEvent* e) {
	return 0;
}

/*		clean up		*/

void clean_up() {
	XCloseDisplay(dsp);
	if (tbar.settings.force_pipes) {
		delete_pipe(tbar.settings.in_file);
		delete_pipe(tbar.settings.out_file);
	}
}