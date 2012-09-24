#ifndef X11_H
	#define X11_h

#include <X11/Xlib.h>
#include <X11/Xatom.h>
/*		x11 atoms		*/

extern Display* dsp;
extern Window root_win;
extern int screen;

extern Atom n_atoms[20];
extern const unsigned int clist_atom;
extern const unsigned int utf8Atom;
extern const unsigned int nameAtom;
extern const unsigned int vis_atom;
extern const unsigned int desk_atom;
extern const unsigned int deskname_atom;
extern const unsigned int curdesk_atom;
extern const unsigned int pid_atom;
extern const unsigned int desknum_atom;

void add_event_request(Window wid, int mask=StructureNotifyMask|PropertyChangeMask);
unsigned char* get_xprop(Window win_id, char* prop_name, Atom prop_type, unsigned long* out_num=NULL);
unsigned char* get_xprop(Window win_id, Atom prop_atom, Atom prop_type, unsigned long* out_num=NULL);
unsigned long* get_desktop_for(Window win_id);
char* get_window_title(Window win_id);
Window* get_client_list(unsigned long* out_num);

#endif