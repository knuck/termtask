/*
	termtask - Terminal-based task bar
	Copyright (C) 2012  Benjamin MdA

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <functional>
#include <algorithm>

/*
# termtask

#extra-sector-fmt
%D = num_workspaces
%n = num_windows
%c = cur_desk

#outputfmt
"ẅ x f"
%w = window name
%x = hex id
%i = dec id
%d = desktop id
%p = pid
%z = z-order

$ = group by, eg:

"%w %x" would output
WINDOWS:
<window-name> <window-id>

"$w%w %x" would output
WINDOWS:
TITLE:<window-name> WID:<window-id>

With a proper sector formatting and group by, you can achieve basic sgml output:
Sector formatting: "<%s>%c</%s>"
Output formatting: "$w%w %p"
<WINDOWS>
<WINDOW>
<TITLE>Some window</TITLE>
<PID>23789</PID>
</WINDOW>
</WINDOWS>

Even more so with desktop formatting such as "%t"
<DESKTOP>
<TITLE>Desktop title</TITLE>
<WINDOWS>
<WINDOW>
<TITLE>Some window</TITLE>
<PID>23789</PID>
</WINDOW>
</WINDOWS>
</DESKTOP>

$w = group by window
$d = group by desktop
$p = group by pid
$z = group by z-order

*/

/*		help 		*/

#define VERSION "0.1f"
#define HELP "termtask " VERSION "\n" \
"Usage: termtask [options]...\n" \
"\n" \
"General options\n" \
"  -c, --config=<FILE>      Read configuration from FILE, ignoring other arguments.\n" \
"  -d, --deaf               Ignore commands from input file.\n" \
"  -g, --foreground         Run in foreground. Without this option, termtask will\n" \
"                           daemonize on startup. If -i is set, this is ignored.\n" \
"  -i, --interactive        Start ncurses interactive mode.\n" \
"  -s, --stdout             Output to stdout instead of using a file. Cannot be\n" \
"                           used with -i.\n" \
"\n" \
"I/O control\n" \
"  -p, --handle-pipes       Treat input and output files as named pipes, handling\n" \
"                           creation and deletion.\n" \
"  -I, --input=<FILE>       Path to read commands from. The default is\n" \
"                           /tmp/termtask/in. If used with -d, it has no effect.\n" \
"  -O, --output=<FILE>      Path to write output to. The default is\n" \
"                           /tmp/termtask/out. If used with -s, it has no effect.\n" \
"\n" \
"Formatting\n" \
"  -f, --format=<FMT>       Format string for window output. This is equivalent to\n" \
"                           --set WINDOW <FMT>. Defaults to \"%w\\n\"\n"\
"  -w, --workspace=<FMT>    Format string for desktop output. This is equivalent to\n" \
"                           --set DESKTOP <FMT>. Defaults to \"%t\\n\"\n\n"\
"  -S, --sector=<FMT>       Format string for sector output. This is equivalent to\n" \
"                           --set SECTOR <FMT>. Defaults to \"%s\\n%c\\4\"\n"\
"  -t, --set <SECTOR> <FMT> Sets format string for SECTOR. If SECTOR is not one of\n" \
"                           the default sectors, only global formatting will be\n" \
"                           available. For more information on sectors, refer\n" \
"                           to the manual.\n" \
"\n" \
" For more information on format strings, please refer to the manual.\n" \
"\n" \
"Debugging\n" \
"  -v, --verbose [LEVEL]    Set debug information output level: \n" \
"                             0: Output nothing.\n" \
"                             1: Output debugging information to stderr [default].\n" \
"                             2: Output debugging information and the processed.\n" \
"                                output to stderr.\n" \
"                           The LEVEL value is optional, and if left in blank,\n" \
"                           it'll be set to 2.\n" \
"\n" \
"Misc\n" \
"  -h, --help               Show this help. \n" \
"\n" \
"\n" \
"Author, current maintainer: Benjamin MdA <knuckvice@gmail.com>\n"\
"Website: https://github.com/knuck/termtask\n"\
"Released under the GNU General Public License.\n"\
"Copyright (C) 2012"


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
	std::string sector_fmt;
	int verbose_level;
};

struct workspace {
	std::string title;
	int pos;
};
struct root_win_data {
	long num_desktops;
	long current_desk;
	std::vector<workspace> workspaces;
};
struct taskbar {
	root_win_data root_data;
	rt_settings settings;
	std::vector<task> ordered_tasks;
};

typedef std::string(*format_func) (std::string fmt, void* data);

struct str_func_wrapper {
	format_func fmt_func;
	std::string arg_fmt;
	void* arg_data;
	std::string operator()() {
		return fmt_func(arg_fmt,arg_data);
	}
};



/*		x11 stuff		*/

Display* dsp;
Window root_win;
int screen;

/*		x11 atoms		*/

Atom n_atoms[20];
const unsigned int clist_atom = 0;
const unsigned int utf8Atom = 1;
const unsigned int nameAtom = 2;
const unsigned int vis_atom = 3;
const unsigned int desk_atom = 4;
const unsigned int deskname_atom = 5;
const unsigned int curdesk_atom = 6;
const unsigned int pid_atom = 7;
const unsigned int desknum_atom = 8;

/*		linux stuff		*/

int npipe;

/*		runtime stuff		*/

taskbar tbar;
std::map<std::string,str_func_wrapper> sector_list;

/*		function prototypes		*/

void fail(const char*);
void add_event_request(Window wid, int mask=StructureNotifyMask|PropertyChangeMask);
unsigned char* get_xprop(Window win_id, char* prop_name, Atom prop_type, unsigned long* out_num=NULL);
unsigned char* get_xprop(Window win_id, Atom prop_atom, Atom prop_type, unsigned long* out_num=NULL);
unsigned long* get_desktop_for(Window win_id);
char* get_window_title(Window win_id);
Window* get_client_list(unsigned long* out_num);
void update_desktop_names();
void update_desktop_num();
void update_current_desktop();
void update_desktop_data();
void build_client_list_from_scratch();
int find_tbar_window(Window wid, taskbar* taskbar=&tbar);
void char_array_list_to_vector(char* list, std::vector<std::string>& out, unsigned long nitems);
int get_pid(Window wid);
std::string get_cmd_line(int pid);
int open_pipe();
void close_pipe();
std::string format_task_string(std::string fmt, task& t);

std::string build_tasks_string(std::string fmt, void* ptr);
std::string build_sector_string(std::string sector, std::string fmt, str_func_wrapper& format_content);
void print_taskbar_fmt();
std::string build_workspace_string(std::string fmt, void* ptr);

int init_atoms();
int handle_error(Display* dsp, XErrorEvent* e);
void init_rt_struct();
void init();
void init_x_connection();
void clean_up();

/*		basic fallback		*/

void fail(const char* msg) {
	printf("%s\n", msg);
	clean_up();
	exit(1);
}

void sig_exit(int param) {
	clean_up();
	signal(SIGINT,SIG_DFL);
	raise(SIGINT);
	exit(0);
}


/*		x11 wrappers		*/

void add_event_request(Window wid, int mask /* StructureNotifyMask|PropertyChangeMask */) {
	XSelectInput(dsp,wid,mask);
}

unsigned char* get_xprop(Window win_id, char* prop_name, Atom prop_type, unsigned long* out_num /* NULL */) {
	int format;
	unsigned long nitems,after;
	unsigned char* data = NULL;
	Atom ret_type;
	Atom targ_atom = XInternAtom(dsp,prop_name,true);
	if (Success == XGetWindowProperty(dsp, win_id, targ_atom, 0, 65536,
			false, prop_type, &ret_type, &format,
			&nitems, &after, &data)) {
		if (out_num) *out_num = nitems;
		return data;
	}
	return NULL;
}


unsigned char* get_xprop(Window win_id, Atom prop_atom, Atom prop_type, unsigned long* out_num /* NULL */) {
	int format;
	unsigned long nitems,after;
	unsigned char* data = NULL;
	Atom ret_type;
	if (Success == XGetWindowProperty(dsp, win_id, prop_atom, 0, 65536,
			false, prop_type, &ret_type, &format,
			&nitems, &after, &data)) {
		if (out_num) *out_num = nitems;
		return data;
	}
	return NULL;
}


/*		get_xprop wrappers		*/

unsigned long* get_desktop_for(Window win_id) {
	unsigned long* retval = (unsigned long *)get_xprop(win_id,n_atoms[desk_atom],XA_CARDINAL);
	return (unsigned long *)get_xprop(win_id,n_atoms[desk_atom],XA_CARDINAL);
}

char* get_window_title(Window win_id) {
	return (char*)get_xprop(win_id,n_atoms[nameAtom],n_atoms[utf8Atom]);
}

Window* get_client_list(unsigned long* out_num) {
	return (Window*)get_xprop(root_win,n_atoms[clist_atom],XA_WINDOW,out_num);
}

/*		virtual desktops		*/

void update_desktop_names() {
	unsigned long nitems;
	char* desk_names = (char*)get_xprop(root_win,n_atoms[deskname_atom],n_atoms[utf8Atom],&nitems);
	std::vector<std::string> converted_vec;
    char_array_list_to_vector(desk_names,converted_vec, nitems);
    int i = 0;
    while (converted_vec.size() > 0) {
    	tbar.root_data.workspaces.push_back({converted_vec.at(0),i});
    	converted_vec.erase(converted_vec.begin());
    	i++;
    }
}

void update_desktop_num() {
	tbar.root_data.num_desktops = (long)get_xprop(root_win,n_atoms[desknum_atom],XA_CARDINAL,NULL);
}

void update_current_desktop() {
	tbar.root_data.current_desk = (long)get_xprop(root_win,n_atoms[curdesk_atom],XA_CARDINAL,NULL);
}

void update_desktop_data() {
	update_current_desktop();
	update_desktop_num();
	update_desktop_names();
}

/*		top-level windows		*/

void build_client_list_from_scratch() {
	tbar.ordered_tasks.clear();
	unsigned long num_items;
	XSync(dsp,true);
	Window* win_ptr = get_client_list(&num_items);
	if (win_ptr != NULL) {
		//printf("Got client list...\n");
		for (unsigned long i = 0; i < num_items; i++) {
			char* wtitle = get_window_title(win_ptr[i]);
			if (wtitle != NULL) {
				unsigned long* desk_id = get_desktop_for(win_ptr[i]);
				signed long targ_desk = (signed long)desk_id==ALL_DESKTOPS?ALL_DESKTOPS:(signed long)*desk_id;
				task t;
					t.wid = win_ptr[i];
					t.title = wtitle;
					t.pid = get_pid(t.wid);
					t.command = get_cmd_line(t.pid);
					t.desktop = targ_desk;
				tbar.ordered_tasks.push_back(t);
				XFree(desk_id);
				//printf("Adding request for a window %s\n", wtitle);
				add_event_request(win_ptr[i]);
				XFree(wtitle);
			}
		}
		//printf("Got all clients\n");
		XFree(win_ptr);
	}
}

/*		helpers		*/

int find_tbar_window(Window wid, taskbar* taskbar /* &tbar */) {
	int i = 0;
	for (auto it = taskbar->ordered_tasks.begin(); it != taskbar->ordered_tasks.end(); it++) {
		if ((*it).wid == wid) {
			return i;
		}
		i+=1;
	}
	return WINDOW_NOT_REGGED;
}

void char_array_list_to_vector(char* list, std::vector<std::string>& out, unsigned long nitems) {
	out.clear();
	std::string namebuf;
    int i = 0;
    while (i < nitems) {
    	namebuf = list+i;
    	out.push_back(namebuf);
    	i += strlen(list+i)+1;
    }
}

/*		pid and command-line		*/

int get_pid(Window wid) {
	int pid = 0;
	unsigned char *r = get_xprop(wid,n_atoms[pid_atom],XA_CARDINAL);
	if (r != 0) {
		pid = (r[1]*256)+r[0];
		XFree(r);
	}
	return pid;
}


#define PROC std::string("/proc/")
#define CMDLINE std::string("/cmdline")
std::string get_cmd_line(int pid) {
	char buf[10];
	sprintf(buf,"%d",pid);
	std::string targ_file = PROC+buf+CMDLINE;
	std::string retval;
	FILE *f = fopen(targ_file.c_str(),"rb");
	if (f) {
		char cur_c;
		while (!feof(f)) {
			fread(&cur_c,sizeof(char),1,f);
			retval+= cur_c;
		}
		fclose(f);
	}
	return retval;
}

/*		io/pipe handling		*/

bool create_pipe(char* path) {
	if (mkfifo(path,0666) != 0) {
		if (errno != EEXIST) return false;
	}
	return true;
}

bool delete_pipe(char* path) {
	if (unlink(path) != 0) {
		return false;
	}
	return true;
}

void write_raw_sector(FILE* file, std::string secname, unsigned int length) {
	fprintf(file, "%s", secname.c_str());
	fwrite(&length,4,1,file);
}

/*		formatting		*/

template <typename T> bool generic_format_string(std::string targ_str, std::map<std::string, std::function<std::string(T&)>> allowed_fmt) {
	bool is_in_fmt = false;
	std::string current_format = "";
	for (auto it = targ_str.begin(); it != targ_str.end(); it++) {
		if (is_in_fmt) {
			if (current_format[0] == *it) {
				current_format += "";
			} else {
				printf("searching for %s\n", (current_format+(*it)).c_str());
				if (allowed_fmt.find(current_format+(*it)) != allowed_fmt.end()) {
					current_format = "";
					//allowed_fmt[current_format+(*it)]()
				} else {
					printf("failed\n");
					return false;
				}
			}
			is_in_fmt = false;
		} else {
			if (allowed_fmt.end()!=find_if(allowed_fmt.begin(),allowed_fmt.end(),[it] (std::pair<const std::string, std::function<std::string(T&)>>& sit) -> bool {
				return sit.first[0] == *it;
			})) {
				is_in_fmt = true;
				current_format += *it;
			}
		}
	}
	return true;
}

bool eval_format_string(std::string targ_str, std::string allowed_fmt, std::string formatters) {
	bool is_in_fmt = false;
	char current_format = '\0';
	for (auto it = targ_str.begin(); it != targ_str.end(); it++) {
		if (is_in_fmt) {
			if (current_format == *it || allowed_fmt.find(*it) != std::string::npos) {
				current_format = '\0';
			} else {
				return false;
			}
			is_in_fmt = false;
		} else {
			if (formatters.find(*it) != std::string::npos) {
				is_in_fmt = true;
				current_format = *it;
			}
		}
	}
	return true;
}
std::map<std::string,std::function<std::string(task&)>> win_fmts = {
	{"%w",[](task &t) {
						char buf[256];
						sprintf(buf,"%.*s",tbar.settings.max_title_size,t.title.c_str());
						return std::string(buf);
					   }
	},
	{"%d",[](task &t) {
						char buf[20];
						sprintf(buf,"%ld",(signed long *)t.desktop);
						return std::string(buf);
					   }
	},
	{"%x",[](task &t) {
						char buf[256];
						sprintf(buf,"0x02%x", t.wid);
						return std::string(buf);
					   }
	},
	{"%X",[](task &t) {
						char buf[256];
						sprintf(buf,"0x02%X", t.wid);
						return std::string(buf);
	}}
};

std::map<std::string,std::function<std::string(workspace&)>> work_fmts = {
	{"%t",[](workspace &wp) {
						return wp.title;
					   }
	},
	{"%d",[](workspace &wp) {
						char buf[20];
						sprintf(buf,"%ld", (signed long *)wp.pos);
						return buf;
	}}
};

std::map<std::string,std::function<std::string(task&)>> gen_fmts = {
	{"%w",[&](task &t) {
						char buf[256];
						sprintf(buf,"%.*s",tbar.settings.max_title_size,t.title.c_str());
						return std::string(buf);
					   }
	},
	{"%d",[](task &t) {
						char buf[20];
						sprintf(buf,"%ld",(signed long *)t.desktop);
						return std::string(buf);
					   }
	},
	{"%x",[](task &t) {
						char buf[256];
						sprintf(buf,"0x02%x", t.wid);
						return std::string(buf);
					   }
	},
	{"%X",[](task &t) {
						char buf[256];
						sprintf(buf,"0x02%X", t.wid);
						return std::string(buf);
	}}
};
										

std::string format_task_string(std::string fmt, task& t) {
	bool is_in_fmt = false;
	std::string out_str;
	for (auto it = fmt.begin(); it != fmt.end(); it++) {
		if (is_in_fmt) {
			if ((*it) == '%') {
				out_str += '%';
			} else if ((*it) == 'n') {
				char buf[256];
				sprintf(buf,"%.*s", tbar.settings.max_title_size,t.title.c_str());
				out_str += buf;
			} else if ((*it) == 'd') {
				char buf[20];
				sprintf(buf,"%ld", (signed long *)t.desktop);
				out_str += buf;
			} else if ((*it) == 'x') {
				char buf[20];
				sprintf(buf,"0x02%x", t.wid);
				out_str += buf;
			} else if ((*it) == 'X') {
				char buf[20];
				sprintf(buf,"0x02%X", t.wid);
				out_str += buf;
			} else {
				char buf[256];
				out_str = *it;
				sprintf(buf,"Unknown format character: %%%s\n",out_str.c_str());
				fail(buf);
				break;
			}
			is_in_fmt = false;
		} else {
			if ((*it) == '%') {
				is_in_fmt = true;
			} else {
				out_str += *it;
			}
		}
	}
end_fmt:
	return out_str;
}

std::string build_tasks_string(std::string fmt, void* ptr) {
	std::string out_str;
	std::vector<task>* tasks = (std::vector<task>*)ptr;
	for (auto it = tasks->begin(); it != tasks->end(); it++) {
		out_str += format_task_string(fmt,*it);
	}
	return out_str;
}

std::string build_sector_string(std::string sector, std::string fmt, str_func_wrapper& format_content) {
	bool is_in_fmt = false;
	std::string out_str;
	for (auto it = fmt.begin(); it != fmt.end(); it++) {
		if (is_in_fmt) {
			if ((*it) == '%') {
				out_str += '%';
			} else if ((*it) == 's') {
				out_str += sector;
			} else if ((*it) == 'c') {
				out_str += format_content();
			} else {
				char buf[256];
				out_str = *it;
				sprintf(buf,"Unknown format character: %%%s\n",out_str.c_str());
				fail(buf);
				break;
			}
			is_in_fmt = false;
		} else {
			if ((*it) == '%') {
				is_in_fmt = true;
			} else {
				out_str += *it;
			}
		}
	}
end_fmt:
	return out_str;
}

void print_sector_list() {
	std::string seclist;
	char buf[256];
	int total_size = 0;
	for (auto it = sector_list.begin(); it != sector_list.end(); it++) {
		total_size+=sprintf(buf,"%s\n", it->first.c_str());
		seclist += buf;
		//seclist += "\4\0";
	}

	FILE *f = fopen(tbar.settings.out_file,"w");
		write_raw_sector(f,"SECTORLIST",total_size);
		printf("SECTORLIST");
		printf("%d\n", total_size);
		fwrite(seclist.c_str(),1,seclist.size(),f);
		printf("%s\n", seclist.c_str());
	fclose(f);
}

void print_taskbar_fmt() {
	FILE *f = fopen(tbar.settings.out_file,"w");
	for (auto it = sector_list.begin(); it != sector_list.end(); it++) {
		//fprintf(f,"%s\n", build_sector_string(it->first, tbar.settings.sector_fmt, it->second).c_str());
		
	}
	fclose(f);
}

std::string build_general_string(std::string fmt, void* ptr);

void set_sector_fmt(std::string secname, std::string fmt) {
	if (secname == "WINDOWS") {
		sector_list["WINDOWS"] = {build_tasks_string, fmt, &tbar.ordered_tasks};
	} else if (secname == "DESKTOPS") {
		sector_list["DESKTOPS"] = {build_workspace_string, fmt, &tbar.root_data.workspaces};
	} else {
		sector_list[secname] = {build_general_string, fmt, &tbar.root_data.workspaces};
	}
}

std::string format_workspace_string(std::string fmt, workspace& wp) {
	std::string out_str;
	bool is_in_fmt = false;
	for (auto it = fmt.begin(); it != fmt.end(); it++) {
		if (is_in_fmt) {
			if ((*it) == '%') {
				out_str += '%';
			} else if ((*it) == 't') {
				out_str += wp.title;
			} else if ((*it) == 'd') {
				char buf[20];
				sprintf(buf,"%ld", (signed long *)wp.pos);
				out_str += buf;
			} else {
				char buf[256];
				out_str = *it;
				sprintf(buf,"Unknown format character: %%%s\n",out_str.c_str());
				fail(buf);
				break;
			}
			is_in_fmt = false;
		} else {
			if ((*it) == '%') {
				is_in_fmt = true;
			} else {
				out_str += *it;
			}
		}
	}
end_fmt:
	return out_str;
}

std::string build_workspace_string(std::string fmt, void* ptr) {
	std::string out_str;
	std::vector<workspace>* workspaces = (std::vector<workspace>*)ptr;
	for (auto it = workspaces->begin(); it != workspaces->end(); it++) {
		out_str += format_workspace_string(fmt,*it);
	}
	return out_str;
}

std::string build_general_string(std::string fmt, void* ptr) {
	std::string out_str;
	std::vector<workspace>* workspaces = (std::vector<workspace>*)ptr;
	for (auto it = workspaces->begin(); it != workspaces->end(); it++) {
		out_str += format_workspace_string(fmt,*it);
	}
	return out_str;
}

/*		main stuff		*/

int init_atoms() {
	for (int i = 0; i < sizeof(n_atoms)/sizeof(Atom); i++) {
		n_atoms[i] = None;
	}
	n_atoms[vis_atom] = XInternAtom(dsp,"_NET_WM_VISIBLE_NAME",true);
	n_atoms[nameAtom] = XInternAtom(dsp,"_NET_WM_NAME",true);
	n_atoms[utf8Atom] = XInternAtom(dsp,"UTF8_STRING",true);
	n_atoms[clist_atom] = XInternAtom(dsp,"_NET_CLIENT_LIST",true);
	if (n_atoms[clist_atom] == None){
		n_atoms[clist_atom] = XInternAtom(dsp,"_WIN_CLIENT_LIST",true);
		if (n_atoms[clist_atom] == None){
			fail("Could not retrieve window list");
		}
	}
	n_atoms[desk_atom] = XInternAtom(dsp,"_NET_WM_DESKTOP",true);
	n_atoms[deskname_atom] = XInternAtom(dsp,"_NET_DESKTOP_NAMES",true);
	n_atoms[curdesk_atom] = XInternAtom(dsp,"_NET_CURRENT_DESKTOP",true);
	n_atoms[pid_atom] = XInternAtom(dsp,"_NET_WM_PID",true);
	n_atoms[desknum_atom] = XInternAtom(dsp,"_NET_NUMBER_OF_DESKTOPS",true);
	
	return 0;
}

int handle_error(Display* dsp, XErrorEvent* e) {
	return 0;
}

void init_x_connection() {
	dsp = XOpenDisplay(NULL);
	screen = DefaultScreen(dsp);
	root_win = RootWindow(dsp, screen);
	XSetErrorHandler(handle_error);
}

void set_in_file(char* path) {
	strcpy(tbar.settings.in_file, path);
}

void set_out_file(char* path) {
	strcpy(tbar.settings.out_file, path);
}

void set_in_file(const char* path) {
	strcpy(tbar.settings.in_file, path);
}

void set_out_file(const char* path) {
	strcpy(tbar.settings.out_file, path);
}

void init_rt_struct() {
	tbar.settings.max_title_size = 255;
	set_sector_fmt("WINDOWS","%n\n");
	set_sector_fmt("DESKTOPS","%t\n");
	set_sector_fmt("GENERAL","%c\n");
	tbar.settings.deaf = false;
	tbar.settings.interactive = false;
	tbar.settings.force_pipes = false;
	tbar.settings.stdout = false;
	set_in_file("/tmp/termtask/in");
	set_out_file("/tmp/termtask/out");
	tbar.settings.sector_fmt = "\4%s: %c";
	tbar.settings.verbose_level = 0;
}

void init() {
	bool retval = generic_format_string("%w %d", win_fmts);
	retval = generic_format_string("%w %F", win_fmts);
	retval = generic_format_string("%w %X", win_fmts);
	init_rt_struct();
	add_event_request(root_win,SubstructureNotifyMask|PropertyChangeMask);
	update_desktop_data();
	build_client_list_from_scratch();
}

void clean_up() {
	XCloseDisplay(dsp);
	if (tbar.settings.force_pipes) {
		delete_pipe(tbar.settings.in_file);
		delete_pipe(tbar.settings.out_file);
	}
}

void parse_args(int argc, char* argv[]) {
	static option long_options[] = {
		{"config",required_argument,NULL,'c'},
		{"deaf",no_argument,NULL,'d'},
		{"interactive",no_argument,NULL,'i'},
		{"pipes",no_argument,NULL,'p'},
		{"input",required_argument,NULL,'I'},
		{"output",required_argument,NULL,'O'},
		{"format",required_argument,NULL,'f'},
		{"foreground",required_argument,NULL,'F'},
		{"workspace",required_argument,NULL,'w'},
		{"verbose",optional_argument,NULL,'v'},
		{"stdout",no_argument,NULL,'s'},
		{"sector",required_argument,NULL,'S'},
		{"set",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{0,0,0}
	};
	int c;
	int option_index;
	std::string cur_sec_name = "";
	while ((c = getopt_long(argc, argv, "-dipshFS:I:O:t:f:w:c:v::",long_options,&option_index)) != -1) {
		switch (c) {
			case 1: {
				if (cur_sec_name != "") {
					if (eval_format_string(optarg,"Dnc","%") == false) fail("Invalid format string.");
					set_sector_fmt(cur_sec_name,optarg);
					cur_sec_name = "";
				}
				break;
			}
			case 'd': {
				tbar.settings.deaf = true;
				break;
			}
			case 'i': {
				tbar.settings.interactive = true;
				break;
			}
			case 'p': {
				tbar.settings.force_pipes = true;
				break;
			}
			case 's': {
				tbar.settings.stdout = true;
				break;
			}
			case 't': {
				cur_sec_name = optarg;
				break;
			}
			case 'I': {
				set_in_file(optarg);
				break;
			}
			case 'O': {
				set_out_file(optarg);
				break;
			}
			case 'f': {
				if (eval_format_string(optarg,"wxidopD","%$") == false) fail("Invalid format string.");
				sector_list["WINDOWS"] = {build_tasks_string, optarg, &tbar.ordered_tasks};
				
				break;
			}
			case 'w': {
				if (eval_format_string(optarg,"tic","%") == false) fail("Invalid workspace format string.");
				sector_list["DESKTOPS"] = {build_workspace_string, optarg, &tbar.root_data.workspaces};
				break;
			}
			case 'S': {
				if (eval_format_string(optarg,"sc","%") == false) fail("Invalid sector format string.");
				tbar.settings.sector_fmt = optarg;
				break;
			}
			case 'v': {
				if (optarg) {
					tbar.settings.verbose_level = atoi(optarg);
				} else {
					tbar.settings.verbose_level = 2;
				}
				break;
			}
			case 'h': {
				printf("%s\n", HELP);
				break;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	
	init_x_connection();
	init_atoms();
	init();
	signal(SIGINT,sig_exit);
	parse_args(argc,argv);
	if (tbar.settings.force_pipes == true) {
		if (!create_pipe(tbar.settings.in_file)) fprintf(stderr, "Could not create pipe at %s\n", tbar.settings.in_file);
		if (!create_pipe(tbar.settings.out_file)) fprintf(stderr, "Could not create pipe at %s\n", tbar.settings.out_file);
	}
	print_taskbar_fmt();
	print_sector_list();
	XEvent e;
	
	while (1) {
		while (XPending (dsp)) {
			XNextEvent(dsp, &e);
			switch (e.type) {
				case PropertyNotify: {
					XSync(dsp,false);
					XPropertyEvent* de = (XPropertyEvent*)&e;
					if (find_tbar_window(de->window) != WINDOW_NOT_REGGED) {
						build_client_list_from_scratch();
						print_taskbar_fmt();
					} else if (de->window == root_win) {

					}
					break;
				}
				case DestroyNotify: {
					XSync(dsp,false);
					XDestroyWindowEvent* de = (XDestroyWindowEvent*)&e;
					if (find_tbar_window(de->window) != WINDOW_NOT_REGGED) {
						build_client_list_from_scratch();
						print_taskbar_fmt();
					}
					break;
				}
				case CreateNotify: {
					XSync(dsp,false);
					XCreateWindowEvent* ce = (XCreateWindowEvent*)&e;
					Window wid;
					build_client_list_from_scratch();
					print_taskbar_fmt();
				}
			}
		}
		usleep(1000);
	}
end:
	clean_up();
	return 0;
}