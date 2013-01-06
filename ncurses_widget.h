#ifndef _NCURSES_WIDGET_H
	#define _NCURSES_WIDGET_H

#include <ncurses.h>
#include <functional>
#include <vector>

struct ncurses_widget {
	int id;
	int row, col, z_index;
	std::function<int()> width, height;
	int position;
	bool show;
	int fg_color, bg_color;
	int border;
	//const ncurses_widget& parent;
	//std::vector<ncurses_widget> childs;
	std::function<void(char)> on_key_input;
	std::function<void(int,int)> on_mouse_input;
};

void ncurses_widget_move(ncurses_widget& wd, int x, int y);
void ncurses_widget_show(ncurses_widget& wd);
void ncurses_widget_hide(ncurses_widget& wd);
void ncurses_widget_pos_up(ncurses_widget& wd);
void ncurses_widget_pos_down(ncurses_widget& wd);
void ncurses_widget_pos_set(ncurses_widget& wd, int z);

#endif