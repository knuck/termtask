#ifndef _TYPES_H
	#define _TYPES_H
#include <X11/Xlib.h>
#include <string>
#include <vector>
#include <map>
#include "workspace.h"
#include "task.h"
#include "settings.h"

/*		defines		*/

#define MAX_PROPERTY_VALUE_LEN 4096
#define ALL_DESKTOPS -1
#define WINDOW_NOT_REGGED -1

struct root_win_data {
	long num_desktops;
	long current_desk;
	long current_window;
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
//extern std::map<std::string,std::string> sector_list;

#endif
