#include "ncurses.h"

WINDOW* stdscreen;
term_settings terminal_settings;

void update_terminal_size() {
	int max_x, max_y;
	getmaxyx(stdscreen,terminal_settings.rows,terminal_settings.cols);
}

void get_terminal_settings(term_settings& terminal_settings) {
	update_terminal_size();
	terminal_settings.colors = has_colors();
}



void ncurses_draw(ncurses_widget& wd) {
	int targ_width = wd.width() > terminal_settings.cols ? terminal_settings.cols : wd.width();
	int targ_height = wd.height() > terminal_settings.rows ? terminal_settings.rows : wd.height();
	for (int i = 0; i < targ_width; i++) {
		int cur_x = wd.col+i;
		for (int j = 0; j < targ_height; j++) {
			int cur_y = wd.row+j;
			if (is_border_area())
				mvaddch(cur_y,cur_x,'+');
		}
	}
}

#define ARRAY_SIZE(a) 5
char *choices[] = {
                        "Choice Uma",
                        "Choice 2",
                        "Choice 3",
                        "Choice 4",
                        "Exit",
                  };
char *choices1[] = {
                        "Choice 5",
                        "Choice 2",
                        "Choice 3",
                        "Choice 4",
                        "Choice 5",
                        "Exit",
                  };

void free_menu_items(MENU* some_menu) {

}

ITEM** init_items(char** menu_choices) {
	ITEM **my_items;
	ITEM *cur_item;
	int n_choices = ARRAY_SIZE(menu_choices);
	my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));
	for(int i = 0; i < n_choices; ++i)
	        my_items[i] = new_item(menu_choices[i], menu_choices[i]);
	my_items[n_choices] = (ITEM *)NULL;
	return my_items;
}

MENU* init_menu(char** menu_choices) {
	int c;				
	MENU *my_menu;
	my_menu = new_menu((ITEM **)init_items(menu_choices));
	return my_menu;
}