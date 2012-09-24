#ifndef _X11_H
	#define _X11_h

#include <X11/Xlib.h>
#include <X11/Xatom.h>
/*		x11 atoms		*/

extern Display* dsp;
extern Window root_win;
extern int screen;

extern Atom n_atoms[20];
const unsigned int clist_atom = 0;
const unsigned int utf8Atom = 1;
const unsigned int nameAtom = 2;
const unsigned int vis_atom = 3;
const unsigned int desk_atom = 4;
const unsigned int deskname_atom = 5;
const unsigned int curdesk_atom = 6;
const unsigned int pid_atom = 7;
const unsigned int desknum_atom = 8;

void add_event_request(Window wid, int mask=StructureNotifyMask|PropertyChangeMask);
unsigned char* get_xprop(Window win_id, char* prop_name, Atom prop_type, unsigned long* out_num=NULL);
unsigned char* get_xprop(Window win_id, Atom prop_atom, Atom prop_type, unsigned long* out_num=NULL);
unsigned long* get_desktop_for(Window win_id);
char* get_window_title(Window win_id);
Window* get_client_list(unsigned long* out_num);

#endif