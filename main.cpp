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
	for (auto it = begin(tbar.settings.sector_list); it != end(tbar.settings.sector_list); it++) {
		fprintf(f,"%s\n", generic_format_string(tbar.settings.sector_fmt, sec_fmts, *it).c_str());
		fprintf(f,"next one\n");
	}
	release_out_file();
}

void ncurses_end() {

	endwin();
}
MENU* my_menu;
extern char *choices[];
extern char *choices1[];
ncurses_table main_table;
ncurses_widget simple_box;
ncurses_widget simple_box2;
void ncurses_loop() {
	auto running = true;
	while (running) {
		auto ch = getch();
		if (ch == 'q') {
			running = false;
		} else if (ch == 'a') {
			int cur = item_index(current_item(my_menu));
			free_menu(my_menu);
			my_menu = init_menu(choices1);
			post_menu(my_menu);
			set_current_item(my_menu,*((menu_items(my_menu))+cur));
			refresh();
		} else if (ch == KEY_UP) {
			clear();
			//menu_driver(my_menu, REQ_UP_ITEM);
			ncurses_widget_move(simple_box,simple_box.col,simple_box.row-1);
			
			ncurses_widget_move(simple_box2,simple_box2.col,simple_box2.row+1);
			
			
		} else if (ch == KEY_DOWN) {
			//menu_driver(my_menu, REQ_DOWN_ITEM);
			clear();
			ncurses_widget_move(simple_box,simple_box.col,simple_box.row+1);
			ncurses_widget_move(simple_box2,simple_box2.col,simple_box2.row-1);
			
			
		} else if (ch == KEY_LEFT) {
			clear();
			ncurses_widget_move(simple_box,simple_box.col-1,simple_box.row);
			
			
		} else if (ch == KEY_RIGHT) {
			clear();
			ncurses_widget_move(simple_box,simple_box.col+1,simple_box.row);
			
			
		}
		
		/*
		mvprintw(1,2,"X11 Clients: ");
		attron(COLOR_PAIR(1));
		printw("4");
		attroff(COLOR_PAIR(1));
		mvprintw(2,2,"Unlisted: ");
		attron(COLOR_PAIR(1));
		printw("0");
		attroff(COLOR_PAIR(1));
		move(5,0);
		attron(COLOR_PAIR(2));
		//attron(A_BOLD);
		printw("  PID USER   WID  DESKTOP         Title                        Command               ");
		//attroff(A_BOLD);
		attroff(COLOR_PAIR(2));*/
		ncurses_draw_table(main_table);
		//ncurses_draw(simple_box2);
		doupdate();
		usleep(1000);
	}
}

collumn_collection test = {
	{"First",0},
	{"Second",0},
	{"Third",0}
};

void ncurses_init() {

	stdscreen = initscr();
	keypad(stdscreen, TRUE);
	nodelay(stdscreen, true);
	noecho();
	//auto child_win = subwin(stdscreen,4,4,2,3);
	//box(child_win,0,0);
	//curs_set()
	get_terminal_settings(terminal_settings);
	if (terminal_settings.colors) start_color();
	
	ncurses_table_set_collumns(main_table, test);
	ncurses_widget_move(main_table,2,3);


	//simple_box.width = []() -> int { return 4; };
	//simple_box.height = []() -> int { return 4; };

	//simple_box2.width = []() -> int { return 8; };
	//simple_box2.height = []() -> int { return 6; };

	//second_box.width = []() -> int { return 4; };
	//second_box.height = []() -> int { return 4; };

	//ncurses_widget_move(simple_box,2,3);
	//ncurses_widget_move(simple_box2,2,3);
	use_default_colors();
	
	init_pair(1, COLOR_RED, -1);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	//my_menu = init_menu(choices);
	//post_menu(my_menu);
	refresh();
	
	ncurses_loop();
	ncurses_end();
	//MENU* 
	//
	//
	//menu_driver(my_menu, REQ_DOWN_ITEM);
	

	
	//printf("%d %d\n", get<0>(term_size), get<1>(term_size));
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
					XSync(dsp, false);
					XPropertyEvent* de = (XPropertyEvent*)&e;
					if (find_tbar_window(de->window) != WINDOW_NOT_REGGED) {
						build_client_list_from_scratch();
						print_taskbar_fmt();
					} else if (de->window == root_win) {

					}
					break;
				}
				case DestroyNotify: {
					XSync(dsp, false);
					XDestroyWindowEvent* de = (XDestroyWindowEvent*)&e;
					if (find_tbar_window(de->window) != WINDOW_NOT_REGGED) {
						build_client_list_from_scratch();
						print_taskbar_fmt();
					}
					break;
				}
				case CreateNotify: {
					XSync(dsp, false);
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