#ifndef _WORKSPACE_H
	#define _WORKSPACE_H

#include <string>

struct workspace {
	std::string title;
	long pos;
};

void update_desktop_names();
void update_desktop_num();
void update_current_desktop();
void update_desktop_data();

#endif