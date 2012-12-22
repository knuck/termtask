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
#include <X11/Xlib.h>
#include <signal.h>
#include "types.h"
#include "helpers.h"
#include "x11.h"
#include "formatting.h"
#include "io.h"
#include "settings.h"
#include "fail.h"
#include "workspace.h"
#include "task.h"
#include "init.h"
#include "ncurses.h"

void output_sector_descriptor() {
	static int num_outs = 0;
	FILE *f = get_sector_file();
		fprintf(f, "%04d %s", num_outs,tbar.settings.sector_fmt.c_str());
		num_outs++;
	release_sector_file();
}

void print_taskbar_fmt() {
	FILE* f = get_out_file();
	for (auto it = tbar.settings.sector_list.begin(); it != tbar.settings.sector_list.end(); it++) {
		fprintf(f,"%s\n", generic_format_string(tbar.settings.sector_fmt, sec_fmts, *it).c_str());
		fprintf(f,"next one\n");
	}
	release_out_file();
}

int main(int argc, char* argv[]) {
	ncurses_init();
	exit(0);
	init_x_connection();
	init_atoms();
	init();
	signal(SIGINT,sig_exit);
	parse_args(argc,argv);
	if (tbar.settings.force_pipes == true) {
		if (!create_pipe(tbar.settings.in_file)) fprintf(stderr, "Could not create pipe at %s\n", tbar.settings.in_file);
		if (!create_pipe(tbar.settings.out_file)) fprintf(stderr, "Could not create pipe at %s\n", tbar.settings.out_file);
	}
	output_sector_descriptor();
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