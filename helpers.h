#ifndef _HELPERS_H
	#define _HELPERS_H


#include <string>
#include <vector>
#include "types.h"
#include <X11/Xlib.h>

std::string operator "" s (const char* p, size_t);
void char_array_list_to_vector(char* list, std::vector<std::string>& out, unsigned long nitems);
int find_tbar_window(Window wid, taskbar* taskbar=&tbar);


#endif