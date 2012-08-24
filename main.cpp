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
#include <sys/stat.h>
#include <string.h>


/*
# termtask
window order

# termtray
tray-icons

# widgets
move

#outputfmt
"ẅ x f"
w = window name
x = hex id
i = dec id
d = desktop id
o = order
*/
#define MAX_PROPERTY_VALUE_LEN 4096


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
	char* named_pipe;
	char* fmt;
	int max_title_size;
};
struct root_win_data {
	long num_desktops;
	long current_desk;
	std::vector<std::string> desk_names;
};
struct taskbar {
	root_win_data root_data;
	rt_settings settings;
	std::vector<task> ordered_tasks;
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


void fail(const char* msg) {
	printf("%s\n", msg);
	XCloseDisplay(dsp);
	exit(1);
}

/*		x11 wrappers		*/

void add_event_request(Window wid, int mask=StructureNotifyMask|PropertyChangeMask) {
	XSelectInput(dsp,wid,mask);
}

unsigned char* get_xprop(Window win_id, char* prop_name, Atom prop_type, unsigned long* out_num=NULL) {
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


unsigned char* get_xprop(Window win_id, Atom prop_atom, Atom prop_type, unsigned long* out_num=NULL) {
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
	int id = 0;
    char_array_list_to_vector(desk_names,tbar.root_data.desk_names, nitems);
    for (auto it = tbar.root_data.desk_names.begin(); it != tbar.root_data.desk_names.end(); it++) {
    	printf("%s\n", (*it).c_str());;
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
				signed long targ_desk = (signed long)desk_id==-1?-1:(signed long)*desk_id;
				tbar.ordered_tasks.push_back({win_ptr[i],i,std::string(wtitle),targ_desk});
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
	signed long cur_desk = -1;
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

int find_tbar_window(Window wid, taskbar* taskbar=&tbar) {
	int i = 0;
	for (auto it = taskbar->ordered_tasks.begin(); it != taskbar->ordered_tasks.end(); it++) {
		if ((*it).wid == wid) {
			return i;
		}
		i+=1;
	}
	return -1;
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

/*		io/pipe handling		*/
int open_pipe() {
	return open("/tmp/dzenesis",O_WRONLY);
}

void close_pipe(int pipe) {
	close(npipe);
}

/*		formatting		*/
std::string fmt_string(task& t, std::string fmt) {
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
				goto end_fmt;
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

void print_task_fmt(std::string fmt="%n\n") {
	npipe = open_pipe();
	std::string out_str;
	for (auto it = tbar.ordered_tasks.begin(); it != tbar.ordered_tasks.end(); it++) {
		out_str += fmt_string(*it,fmt);
	}
	out_str += "\4";
	const char *res_str = out_str.c_str();
	write(npipe,res_str,strlen(res_str));
	close_pipe(npipe);
	printf("%s\n", res_str);
}

void print_taskbar_fmt(std::string fmt="%v %w") {

}

int handle_error(Display* dsp, XErrorEvent* e) {
	return 0;
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

int main(int argc, char* argv[]) {

	dsp = XOpenDisplay(NULL);
	screen = DefaultScreen(dsp);
	root_win = RootWindow(dsp,screen);
	tbar.settings.max_title_size = 255;
	add_event_request(root_win,SubstructureNotifyMask|PropertyChangeMask);
	init_atoms();
	update_desktop_data();
	build_client_list_from_scratch();
	print_task_fmt();
	XSetErrorHandler(handle_error);
	XEvent e;
	
	while (1) {
		while (XPending (dsp)) {
			XNextEvent(dsp, &e);
			switch (e.type) {
				case PropertyNotify: {
					XSync(dsp,false);
					XPropertyEvent* de = (XPropertyEvent*)&e;
					Window windex;
					if ((windex = find_tbar_window(de->window)) != -1) {
						build_client_list_from_scratch();
						print_task_fmt();
					}
					break;
				}
				case DestroyNotify: {
					XSync(dsp,false);
					XDestroyWindowEvent* de = (XDestroyWindowEvent*)&e;
					Window windex;
					if ((windex = find_tbar_window(de->window)) != -1) {
						build_client_list_from_scratch();
						print_task_fmt();
					}
					break;
				}
				case CreateNotify: {
					XSync(dsp,false);
					XCreateWindowEvent* ce = (XCreateWindowEvent*)&e;
					Window wid;
					build_client_list_from_scratch();
					print_task_fmt();
				}
			}
		}
		usleep(1000);
	}
end:
	XCloseDisplay(dsp);
	return 0;
}