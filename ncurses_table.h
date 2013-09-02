#ifndef _NCURSES_TABLE_H
	#define _NCURSES_TABLE_H

#include <ncurses.h>
#include <menu.h>
#include <string>
#include "ncurses_listbox.h"



struct ncurses_table_col {
	std::string text;
	int w;
};

typedef std::vector<ncurses_table_col> collumn_collection;	

struct ncurses_table_row {
	const collumn_collection& parent_table_cols;
	std::vector<std::string> cols;
	std::string operator[](unsigned int index);
	std::string operator[](std::string title);
};

struct ncurses_table : ncurses_widget {
	collumn_collection cols;
	std::vector<ncurses_table_row> rows;
	ncurses_listbox listbox_p;
	ncurses_table_row& operator[] (unsigned int index);
};

void ncurses_table_clear_menu_strings(ncurses_table& tb);
void ncurses_table_clear_menu(ncurses_table& tb);
int ncurses_table_current_index(ncurses_table& tb);
void ncurses_table_select(ncurses_table& tb, int index);
void ncurses_table_set_collumns(ncurses_table& tb, collumn_collection& in_cols);
void ncurses_draw_table(ncurses_table& tb);

#endif