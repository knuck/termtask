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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <vector>
#include <string>
#include <map>
#include <string.h>

#include <signal.h>
#include <functional>

#include "types.h"
#include "helpers.h"
#include "x11.h"
#include "formatting.h"
#include "io.h"
#include "settings.h"
#include "fail.h"

taskbar tbar;
std::map<std::string,std::string> sector_list;

/*
# termtask

#outputfmt
"ẅ x f"
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

$m = group by window
$d = group by desktop
$p = group by pid
$z = group by z-order

*/

/*		function prototypes		*/

void update_desktop_names();
void update_desktop_num();
void update_current_desktop();
void update_desktop_data();
void build_client_list_from_scratch();
int find_tbar_window(Window wid, taskbar* taskbar=&tbar);
int get_pid(Window wid);
std::string get_cmd_line(int pid);
void print_taskbar_fmt();
int init_atoms();
int handle_error(Display* dsp, XErrorEvent* e);
void init_rt_struct();
void init();
void init_x_connection();


/*		virtual desktops		*/

void update_desktop_names() {
	unsigned long nitems;
	char* desk_names = (char*)get_xprop(root_win,n_atoms[deskname_atom],n_atoms[utf8Atom],&nitems);
	std::vector<std::string> converted_vec;
    char_array_list_to_vector(desk_names,converted_vec, nitems);
    int i = 0;
    while (converted_vec.size() > 0) {
    	tbar.workspaces.push_back({converted_vec.at(0),i});
    	converted_vec.erase(converted_vec.begin());
    	i++;
    }
}

void update_desktop_num() {
	unsigned char* ptr = get_xprop(root_win,n_atoms[desknum_atom],XA_CARDINAL,NULL);
	tbar.root_data.num_desktops = (int)*ptr;
	XFree(ptr);
}

void update_current_desktop() {
	unsigned char* ptr = get_xprop(root_win,n_atoms[curdesk_atom],XA_CARDINAL,NULL);
	tbar.root_data.current_desk = (int)*ptr;
	XFree(ptr);
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
				add_event_request(win_ptr[i]);
				XFree(wtitle);
			}
		}
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
	std::string targ_file = "/proc/"s+buf+"/cmdline"s;
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


void write_raw_sector(FILE* file, std::string secname, unsigned int length) {
	fprintf(file, "%s", secname.c_str());
	fwrite(&length,4,1,file);
}

void print_sector_list() {
	std::string seclist;
	char buf[256];
	int total_size = 0;
	for (auto it = sector_list.begin(); it != sector_list.end(); it++) {
		total_size+=sprintf(buf,"%s\n", it->first.c_str());
		seclist += buf;
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
	FILE* f = get_out_file();
	for (auto it = sector_list.begin(); it != sector_list.end(); it++) {
		fprintf(f,"%s\n", generic_format_string(tbar.settings.sector_fmt, sec_fmts, *it).c_str());
		fprintf(f,"next one\n");
	}
	release_out_file();
}



/*		initialization		*/

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

void init_x_connection() {
	dsp = XOpenDisplay(NULL);
	screen = DefaultScreen(dsp);
	root_win = RootWindow(dsp, screen);
	XSetErrorHandler(handle_error);
}

void init_rt_struct() {
	tbar.settings.max_title_size = 255;
	set_sector_fmt("WINDOWS","%w\n");
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
	bool retval = eval_format_string("%w %d", win_fmts);
	retval = eval_format_string("%w %F", win_fmts);
	retval = eval_format_string("%w %X", win_fmts);
	init_rt_struct();
	add_event_request(root_win,SubstructureNotifyMask|PropertyChangeMask);
	update_desktop_data();
	build_client_list_from_scratch();
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
	//print_sector_list();
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