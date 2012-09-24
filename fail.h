#ifndef _FAIL_H
	#define _FAIL_H

#include <X11/Xlib.h>

void fail(const char* msg);
void sig_exit(int param);
int handle_error(Display* dsp, XErrorEvent* e);
void clean_up();

#endif