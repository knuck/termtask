#include "ncurses_widget.h"

const int NCURSES_POS_ABSOLUTE = 0;
const int NCURSES_POS_RELATIVE = 1;

const int NCURSES_BORDER_NORMAL = 0;



void ncurses_widget_move(ncurses_widget& wd, int x, int y) {
	wd.row = x;
	wd.col = y;
}

void ncurses_widget_show(ncurses_widget& wd) {
	wd.show = true;
}

void ncurses_widget_hide(ncurses_widget& wd) {
	wd.show = false;
}

void ncurses_widget_pos_up(ncurses_widget& wd) {
	wd.z_index++;
}

void ncurses_widget_pos_down(ncurses_widget& wd) {
	wd.z_index--;
}

void ncurses_widget_pos_set(ncurses_widget& wd, int z) {
	wd.z_index = z;
}

