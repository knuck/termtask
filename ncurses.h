#ifndef _NCURSES_INCLUDE_H
	#define _NCURSES_INCLUDE_H

#include <ncurses.h>
#include <menu.h>
#include <vector>
#include <string>
#include <functional>
#include "ncurses_widget.h"
#include "ncurses_table.h"
/*	TODO	*/
/*	ListBox (TextInput)
	TextInput (Label)
	SwitchBox (Label)
	Label
	Window frames
	Splitter
*/

#define is_border_area() (										\
							   cur_x == wd.col					\
					     	|| cur_y == wd.row					\
						 	|| cur_x == targ_width+wd.col-1		\
						 	|| cur_y == targ_height+wd.row-1	\
						 )

struct nc_splitter {
	int focus;
	std::vector<WINDOW> windows;
};

/*struct ncurses_textarea : public ncurses_widget {
	std::string text;
};*/

struct term_settings {
	bool colors;
	int rows;
	int cols;
};

extern WINDOW* stdscreen;
extern term_settings terminal_settings;

void update_terminal_size();
void get_terminal_settings(term_settings& terminal_settings);
void ncurses_draw(ncurses_widget& wd);

void free_menu_items(MENU* some_menu);
ITEM** init_items(char** menu_choices);
MENU* init_menu(char** menu_choices);


#endif