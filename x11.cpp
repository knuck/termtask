#include "x11.h"

/*		x11 wrappers		*/

Display* dsp;
Window root_win;
int screen;

/*		x11 atoms		*/

Atom n_atoms[20];


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