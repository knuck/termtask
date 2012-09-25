#include "init.h"
#include "x11.h"
#include "types.h"
#include "settings.h"
#include "formatting.h"
#include "fail.h"

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