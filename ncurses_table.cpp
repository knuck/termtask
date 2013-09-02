#include <algorithm>
#include <vector>
#include "ncurses_table.h"
#include "ncurses_listbox.h"

void ncurses_table_clear_menu_strings(ncurses_table& tb) {
	ncurses_listbox_clear_menu_strings(tb.listbox_p);
}

void ncurses_table_clear_menu(ncurses_table& tb) {
	ncurses_listbox_clear_menu(tb.listbox_p);
}

int ncurses_table_current_index(ncurses_table& tb) {
	return ncurses_listbox_current_index(tb.listbox_p);
}

void ncurses_table_select(ncurses_table& tb, int index) {
	ncurses_listbox_select(tb.listbox_p, index);
}

void ncurses_table_set_items(ncurses_table& tb, std::vector<std::string>& items) {
	//ncurses_listbox_set_items(tb.listbox_p, items);
}

std::string ncurses_table_row::operator[](unsigned int index) {
	return cols[index];
}

std::string ncurses_table_row::operator[](std::string val) {
	auto col_index = find_if(parent_table_cols.begin(), parent_table_cols.end(),
						[val] (ncurses_table_col cur_col) {
							return cur_col.text == val;
						}
					) - parent_table_cols.begin();
	return cols[col_index];
}

ncurses_table_row& ncurses_table::operator[](unsigned int index) {
	return rows[index];
}

void ncurses_draw_table(ncurses_table& tb) {
	//int targ_width = tb.width() > terminal_settings.cols ? terminal_settings.cols : tb.width();
	//int targ_height = tb.height() > terminal_settings.rows ? terminal_settings.rows : tb.height();
	int i = 0;
	int cur_x = tb.col;
	for (auto it = begin(tb.cols); it != end(tb.cols); it++) {
		mvaddstr(tb.row,cur_x,(*it).text.c_str());
		cur_x += (*it).text.length();
	}
}

void ncurses_table_set_collumns(ncurses_table& tb, collumn_collection& in_cols) {
	for (auto it = begin(in_cols); it != end(in_cols); it++) {
		tb.cols.push_back(*it);
	}
}