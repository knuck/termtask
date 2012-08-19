#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

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

void get_window_title(Window win_id) {
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


int init_atoms() {
	for (int i = 0; i < sizeof(n_atoms)/sizeof(Atom); i++) {
		n_atoms[i] = None;
	}
	n_atoms[vis_atom] = XInternAtom(dsp,"_NET_WM_VISIBLE_NAME",true);
	n_atoms[nameAtom] = XInternAtom(dsp,"_NET_WM_NAME",true);
	n_atoms[utf8Atom] = XInternAtom(dsp,"UTF8_STRING",true);
	return 0;
}

int main(int argc, char* argv[]) {
	dsp = XOpenDisplay(NULL);
	screen = DefaultScreen(dsp);
	root_win = RootWindow(dsp,screen);
	//printf("here\n");
	init_atoms();
	//printf("here1\n");
	Atom ptype = XA_WINDOW;
	Atom ret_type;
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
	
	if (Success == XGetWindowProperty(dsp,root_win,n_atoms[clist_atom],0,MAX_PROPERTY_VALUE_LEN/4,false,ptype,&ret_type,&fmt,&num_items,&bytes_left,&data)) {
		Window* clone_ptr = (Window*)data;
		for (int i = 0; i < num_items; i++) {
			//Window cur_win = data[i*sizeof(Window)];
			printf("0x%08x\n", clone_ptr[i]);
			get_window_title(clone_ptr[i]);
		}
		XFree(data);
	}
	XCloseDisplay(dsp);
	return 0;
}