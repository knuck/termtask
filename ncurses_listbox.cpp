#include "ncurses_listbox.h"

void ncurses_listbox_clear_menu_strings(ncurses_listbox& lb) {
	lb.items.clear();
}

void ncurses_listbox_clear_menu(ncurses_listbox& lb) {
	for (auto it = lb.menu_items.begin(); it != lb.menu_items.end(); it++) {
		free_item(*it);
	}
	lb.menu_items.clear();
	ncurses_listbox_clear_menu_strings(lb);
}

int ncurses_listbox_current_index(ncurses_listbox& lb) {
	return item_index(current_item(lb.menu_pointer));
}

void ncurses_listbox_select(ncurses_listbox& lb, int index) {
	int targ = (lb.items.size()-1) < index ? lb.items.size()-1 : index;
	set_current_item(lb.menu_pointer,*((menu_items(lb.menu_pointer))+targ));
}

void ncurses_listbox_set_items(ncurses_listbox& lb, std::vector<std::string>& items) {
	int last_index = ncurses_listbox_current_index(lb);
	ncurses_listbox_clear_menu(lb);
	for (auto it = items.begin(); it != items.end(); it++) {
		ITEM* ptr = new ITEM;
		lb.menu_items.push_back(ptr=new_item((*it).c_str(),(*it).c_str()));
	}
	set_menu_items(lb.menu_pointer, lb.menu_items.data());
	ncurses_listbox_select(lb, last_index);
	lb.items = items;
}