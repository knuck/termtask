#include <ncurses.h>
#include <menu.h>
#include <vector>
#include <tuple>
#include <string>
#include <functional>

/*	TODO	*/
/*	ListBox (TextInput)
	TextInput (Label)
	SwitchBox (Label)
	Label
	Button (Label)
	Window frames
	Splitter
*/

using namespace std;

struct nc_splitter {
	int focus;
	std::vector<WINDOW> windows;
};

const int NCURSES_POS_ABSOLUTE = 0;
const int NCURSES_POS_RELATIVE = 1;

const int NCURSES_BORDER_NORMAL = 0;

struct ncurses_widget {
	int id;
	int row, col, z_index;
	std::function<int()> width, height;
	int position;
	bool show;
	int fg_color, bg_color;
	int border;
	//const ncurses_widget& parent;
	std::vector<ncurses_widget> childs;
};

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

struct ncurses_textarea : public ncurses_widget {
	std::string text;
};


struct term_settings {
	bool colors;
	int rows;
	int cols;
};

WINDOW* stdscreen;
term_settings terminal_settings;

std::tuple<int,int> get_terminal_size() {
	int max_x, max_y;
	getmaxyx(stdscreen,max_x,max_y);
	return make_tuple(max_x,max_y);
}

void get_terminal_settings(term_settings& terminal_settings) {
	auto term_size = get_terminal_size();
	terminal_settings.cols = get<0>(term_size);
	terminal_settings.rows = get<1>(term_size);
	terminal_settings.colors = has_colors();
}

#define is_border_area() cur_x == wd.col || cur_y == wd.row || cur_x == targ_width+wd.col-1 || cur_y == targ_height+wd.row-1

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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
char *choices[] = {
                        "Choice 1",
                        "Choice 2",
                        "Choice 3",
                        "Choice 4",
                        "Exit",
                  };

MENU* init_menu() {
	ITEM **my_items;
	int c;				
	MENU *my_menu;
	int n_choices, i;
	ITEM *cur_item;

	n_choices = ARRAY_SIZE(choices);
	my_items = (ITEM **)calloc(n_choices + 1, sizeof(ITEM *));

	for(i = 0; i < n_choices; ++i)
	        my_items[i] = new_item(choices[i], choices[i]);
	my_items[n_choices] = (ITEM *)NULL;

	my_menu = new_menu((ITEM **)my_items);
	return my_menu;
}

void ncurses_init() {

	stdscreen = initscr();
	nodelay(stdscreen, true);
	//auto child_win = subwin(stdscreen,4,4,2,3);
	//box(child_win,0,0);
	//curs_set()
	get_terminal_settings(terminal_settings);
	refresh();
	char ch;
	ncurses_widget simple_box;
	simple_box.width = []() -> int { return 4; };
	simple_box.height = []() -> int { return 4; };
	ncurses_widget_move(simple_box,2,3);
	bool running = true;
	post_menu(init_menu());
	while (running) {
		if ( (ch = getch()) == 'q' ) {
			running = false;
		}
		ncurses_draw(simple_box);
		refresh();
		usleep(1000);
	}

	endwin();
	//printf("%d %d\n", get<0>(term_size), get<1>(term_size));

}