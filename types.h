#ifndef _TYPES_H
	#define _TYPES_H
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include <map>

/*		defines		*/

#define MAX_PROPERTY_VALUE_LEN 4096
#define ALL_DESKTOPS -1
#define WINDOW_NOT_REGGED -1

struct task {
	Window wid;
	int pos;
	std::string title;
	unsigned int pid;
	std::string command;
	signed long desktop;
};

struct rt_settings {
	int max_title_size;
	// opts
	bool deaf,interactive,force_pipes,stdout;
	char in_file[256],out_file[256];
	FILE *in_file_pointer, *out_file_pointer;
	std::string sector_fmt;
	int verbose_level;
};

struct workspace {
	std::string title;
	long pos;
};

struct root_win_data {
	long num_desktops;
	long current_desk;
};

struct taskbar {
	root_win_data root_data;
	rt_settings settings;
	std::vector<task> ordered_tasks;
	std::vector<workspace> workspaces;
};

typedef std::pair<const std::string,std::string> sector_type;

/*		runtime stuff		*/

extern taskbar tbar;
extern std::map<std::string,std::string> sector_list;

#endif
