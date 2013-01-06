#ifndef _NCURSES_LISTBOX_H
	#define _NCURSES_LISTBOX_H

#include <ncurses.h>
#include <menu.h>
#include <string>
#include "ncurses_widget.h"

struct ncurses_listbox : public ncurses_widget {
	std::vector<std::string> items;
	std::vector<ITEM*> menu_items;
	MENU* menu_pointer;
};

void ncurses_listbox_clear_menu_strings(ncurses_listbox& lb);
void ncurses_listbox_clear_menu(ncurses_listbox& lb);
int ncurses_listbox_current_index(ncurses_listbox& lb);
void ncurses_listbox_select(ncurses_listbox& lb, int index);

#endif