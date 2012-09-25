#ifndef _TASK_H
	#define _TASK_H

#include <string>
#include <X11/Xlib.h>

struct task {
	Window wid;
	int pos;
	std::string title;
	unsigned int pid;
	std::string command;
	signed long desktop;
};

void build_client_list_from_scratch();
int get_pid(Window wid);
std::string get_cmd_line(int pid);

#endif