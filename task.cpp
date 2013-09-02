#include "task.h"
#include "types.h"
#include "x11.h"
#include "helpers.h"

/*		top-level windows		*/

taskbar tbar;

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
					t.exe = get_program_path(t.pid);
					t.desktop = targ_desk;
				tbar.ordered_tasks.push_back(t);
				add_event_request(win_ptr[i]);
				XFree(desk_id);
				XFree(wtitle);
			}
		}
		XFree(win_ptr);
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

std::string read_from_proc(std::string path, int pid) {
	char buf[10];
	sprintf(buf,"%d",pid);
	std::string targ_file = "/proc/"s+buf+path;
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

std::string get_cmd_line(int pid) {
	return read_from_proc("/cmdline"s, pid);
}

std::string get_program_path(int pid) {
	return read_from_proc("/exe"s, pid);
}