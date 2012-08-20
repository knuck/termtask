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


#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <vector>
Display* dsp;
Window root_win;
int screen;
Atom n_atoms[20];
const unsigned int clist_atom = 0;
const unsigned int utf8Atom = 1;
const unsigned int nameAtom = 2;
const unsigned int vis_atom = 3;
/*
# termtask
window list
desktop list
window order

# termtray
tray-icons

# widgets
clock
move

*/
#define MAX_PROPERTY_VALUE_LEN 4096
void fail(const char* msg) {
	printf("%s\n", msg);
	XCloseDisplay(dsp);
	exit(1);
}

struct task {
	Window wid;
	int pos;
	char* title;
};
struct rt_settings {
	int order_by;
	int global_allow_reorder;
	char* named_pipe;
	char* fmt;
};
struct taskbar {
	rt_settings settings;
	std::vector<task> ordered_tasks;
};

void get_xprop(Window win_id) {
	int format;
	unsigned long nitems,after;
	unsigned char* data = NULL;
	Atom ret_type;
	if (Success == XGetWindowProperty(dsp, win_id, n_atoms[nameAtom], 0, 65536,
			false, n_atoms[utf8Atom], &ret_type, &format,
			&nitems, &after, &data)) {
		printf("%s\n",data);
		XFree(data);
	}

}


void get_window_title(Window win_id) {
	int format;
	unsigned long nitems,after;
	unsigned char* data = NULL;
	Atom ret_type;
	if (Success == XGetWindowProperty(dsp, win_id, n_atoms[nameAtom], 0, 65536,
			false, n_atoms[utf8Atom], &ret_type, &format,
			&nitems, &after, &data)) {
		printf("%s |",data);
		XFree(data);
	}

}


int init_atoms() {
	for (int i = 0; i < sizeof(n_atoms)/sizeof(Atom); i++) {
		n_atoms[i] = None;
	}
	n_atoms[vis_atom] = XInternAtom(dsp,"_NET_WM_VISIBLE_NAME",true);
	n_atoms[nameAtom] = XInternAtom(dsp,"_NET_WM_NAME",true);
	n_atoms[utf8Atom] = XInternAtom(dsp,"UTF8_STRING",true);
	return 0;
}

/*
	@i:gui-retrieve-client-titles
		@i:gui-retrieve-clientlist
*/


int main(int argc, char* argv[]) {

	//	@i:x11-open
	dsp = XOpenDisplay(NULL);
	//	@i:x11-get-screen
	screen = DefaultScreen(dsp);
	//	@i:x11-get-root-win
	root_win = RootWindow(dsp,screen);
	init_atoms();
	Atom ptype = XA_WINDOW;
	Atom ret_type;
	//	@i:x11-retrieve-clientlist-atom
	//		-> n_atoms[client_atom]
	n_atoms[clist_atom] = XInternAtom(dsp,"_NET_CLIENT_LIST",true);
	if (n_atoms[clist_atom] == None){
		ptype = XA_CARDINAL;
		n_atoms[clist_atom] = XInternAtom(dsp,"_WIN_CLIENT_LIST",true);
		if (n_atoms[clist_atom] == None){
			fail("Could not retrieve window list");
		}
	}
	int fmt;
	unsigned long num_items, bytes_left;
	unsigned char *data = NULL;
	//	@i:gui-retrieve-clients
	//	$impl-for:x11
	//		@req:x11-atom-valid
	//			-> n_atoms[client_atom]
	//		@i:x11-retrieve-clientlist
	if (Success == XGetWindowProperty(dsp,root_win,n_atoms[clist_atom],0,MAX_PROPERTY_VALUE_LEN/4,false,ptype,&ret_type,&fmt,&num_items,&bytes_left,&data)) {
		Window* clone_ptr = (Window*)data;
	//	@i:gui-retrieve-window-titles
		for (int i = 0; i < num_items; i++) {
	//	@i:gui-retrieve-window-title
			get_window_title(clone_ptr[i]);
			/* desktop ID */
			if ((desktop = (unsigned long *)get_property(disp, client_list[i],
					XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
				desktop = (unsigned long *)get_property(disp, client_list[i],
						XA_CARDINAL, "_WIN_WORKSPACE", NULL);
			}
		}
		XFree(data);
	}
	XCloseDisplay(dsp);
	return 0;
}