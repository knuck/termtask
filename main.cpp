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



/*
# termtask
window order

# termtray
tray-icons

# move
widgets

#outputfmt
"ẅ x f"
%w = window name
%x = hex id
%i = dec id
%d = desktop id
%o = order
%p = pid
%D = num_workspaces
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

#options
-d --deaf, ignore commands from input pipe
-i --interactive, ncurses interactive mode
-p --handle-pipes, treat input and output files as named pipes, handle creation and removal
-I --input <filename>, filename to read from for commands
-O --output <filename>, filename to write status to
-f --format-string= "FORMATSTRING". string formatting for tasks
-w --workspace-string= "FORMATSTRING". string formatting for desktops
-g --foreground don't run as daemon
-v --verbose <LEVEL> output information. default is 0 (output nothing)
	1 = output the same info from pipe to stderr
	2 = output debugging information
	this option only has effect if used with -g
-s --stdout output to stdout
-S --sector-string= "FORMATSTRING", string formatting for sector separators
-h --help, display this help

*/

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
enum OrderType {
	NO_ORDER,
	BY_DESKTOP,
	BY_ALPHA
};

struct rt_settings {
	OrderType order_by;
	int global_allow_reorder;
	int max_title_size;
	// opts
	bool deaf,interactive,force_pipes,stdout;
	char in_file[256],out_file[256];
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
void sort_by(OrderType ord);
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
	//printf("%d\n", nitems);
	std::vector<std::string> converted_vec;
    char_array_list_to_vector(desk_names,converted_vec, nitems);
    int i = 0;
    while (converted_vec.size() > 0) {
    	tbar.root_data.workspaces.push_back({converted_vec.at(0),i});
    	converted_vec.erase(converted_vec.begin());
    	//printf("%s\n", (*it).c_str());;
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
		sort_by(BY_DESKTOP);
		//printf("Got all clients\n");
		XFree(win_ptr);
	}
}

/*		helpers		*/

void sort_by(OrderType ord) {
	std::vector<task> newtask;
	signed long cur_desk = ALL_DESKTOPS;
	while (tbar.ordered_tasks.size() != newtask.size()) {
		for (auto it = tbar.ordered_tasks.begin(); it != tbar.ordered_tasks.end(); it++) {
			if ((*it).desktop == cur_desk) {
				newtask.push_back(*it);
			}
		}
		cur_desk+=1;
	}
	tbar.ordered_tasks = newtask;
}

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

std::string get_cmd_line(int pid) {
	char buf[10];
	sprintf(buf,"%d",pid);
	std::string targ_file = std::string("/proc/")+buf+std::string("/cmdline");
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
	//printf("%s\n", retval.c_str());
	return retval;
}

/*		io/pipe handling		*/

int open_pipe() {
	return open("/tmp/dzenesis",O_WRONLY);
}

void close_pipe(int pipe) {
	close(npipe);
}

void write_pipe_sector(int pipe, std::string secname, unsigned int length) {
	const char* tmp_ptr = secname.c_str();
	write(pipe,tmp_ptr,strlen(tmp_ptr)+1);
	write(pipe,&length,4);
}

void write_pipe(int pipe, std::string sector, std::string& data) {
	const char* tmp_ptr = data.c_str();
	write_pipe_sector(pipe,sector,strlen(tmp_ptr));
	write(pipe,tmp_ptr,strlen(tmp_ptr));
}

void write_pipe(int pipe, std::string sector, const char* data) {
	write_pipe_sector(pipe,sector,strlen(data));
	write(pipe,data,strlen(data));
}

/*		formatting		*/

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

void print_taskbar_fmt() {
	for (auto it = sector_list.begin(); it != sector_list.end(); it++) {
		printf("%s\n", build_sector_string(it->first, "\4%s: %c", it->second).c_str());
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
	root_win = RootWindow(dsp,screen);
	XSetErrorHandler(handle_error);
}

void init_rt_struct() {
	tbar.settings.max_title_size = 255;
	sector_list["WINDOWS"] = {build_tasks_string,"%n\n",&tbar.ordered_tasks};
	sector_list["DESKTOPS"] = {build_workspace_string,"%t\n",&tbar.root_data.workspaces};
	tbar.settings.deaf = false;
	tbar.settings.interactive = false;
	tbar.settings.force_pipes = false;
	tbar.settings.stdout = false;
	strcpy(tbar.settings.in_file,"/tmp/termtask/in");
	strcpy(tbar.settings.out_file,"/tmp/termtask/out");
	tbar.settings.verbose_level = 0;
}

void init() {
	init_rt_struct();
	add_event_request(root_win,SubstructureNotifyMask|PropertyChangeMask);
	update_desktop_data();
	build_client_list_from_scratch();
	//printf("%s\n",build_workspace_string("%t %d\n", &tbar.root_data.workspaces).c_str());
}

void clean_up() {
	XCloseDisplay(dsp);
}

void parse_args(int argc, char* argv[]) {
	static option long_options[] = {
		{"config-file",required_argument,NULL,'c'},
		{"deaf",no_argument,NULL,'d'},
		{"interactive",no_argument,NULL,'i'},
		{"pipes",no_argument,NULL,'p'},
		{"input-file",required_argument,NULL,'I'},
		{"output-file",required_argument,NULL,'O'},
		{"format-string",required_argument,NULL,'f'},
		{"foreground",required_argument,NULL,'F'},
		{"workspace-string",required_argument,NULL,'w'},
		{"verbose",optional_argument,NULL,'v'},
		{"stdout",no_argument,NULL,'s'},
		{"sector-string",required_argument,NULL,'S'},
		{"help",no_argument,NULL,'h'},
		{0,0,0}
	};
	int c;
	int option_index;
	while ((c = getopt_long(argc, argv, "dipshFS:I:O:f:w:c:v::",long_options,&option_index)) != -1) {
		printf ("option %d\n", option_index);
		switch (c) {
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
			case 'I': {
				strcpy(tbar.settings.in_file,optarg);
				break;
			}
			case 'O': {
				strcpy(tbar.settings.out_file,optarg);
				break;
			}
			case 'f': {
				//tbar.settings.interactive = true;
				break;
			}
			case 'w': {
				//tbar.settings.interactive = true;
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
		}
	}
	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
	
	init_x_connection();
	init_atoms();
	init();
	parse_args(argc,argv);
	
	print_taskbar_fmt();
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