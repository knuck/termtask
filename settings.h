#ifndef _SETTINGS_H
	#define _SETTINGS_H

#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>

void set_sector_fmt(std::string secname, std::string fmt);
void set_in_file(char* path);
void set_out_file(char* path);
void set_in_file(const char* path);
void set_out_file(const char* path);
void set_sector_file(char* path);
void set_sector_file(const char* path);
void parse_args(int argc, char* argv[]);


struct rt_settings {
	int max_title_size;
	// opts
	bool deaf,interactive,force_pipes,stdout,no_transient;
	char in_file[256],out_file[256], sector_file[256];
	FILE *in_file_pointer, *out_file_pointer, *sec_file_pointer;
	std::string sector_fmt;
	std::map<std::string,std::string> sector_list;
	std::string group_sort_fmt;
	int verbose_level;
};

/*		help 		*/

#define VERSION "0.2"
#define HELP "termtask " VERSION "\n" \
"Usage: termtask [options]...\n" \
"\n" \
"General options\n" \
"  -c, --config=<FILE>      Read configuration from FILE, ignoring other arguments.\n" \
"  -d, --deaf               Ignore commands from input file.\n" \
"  -g, --foreground         Run in foreground. Without this option, termtask will\n" \
"                           daemonize on startup. If -i is set, this is ignored.\n" \
"  -i, --interactive        Start ncurses interactive mode.\n" \
"  -n, --no-transient       Ignores transient windows.\n" \
"  -s, --stdout             Output to stdout instead of using a file. Cannot be\n" \
"                           used with -i.\n" \
"\n" \
"I/O control\n" \
"  -p, --handle-pipes       Treat input and output files as named pipes, handling\n" \
"                           creation and deletion.\n" \
"  -I, --input=<FILE>       Path to read commands from. The default is\n" \
"                           /tmp/termtask/in. If used with -d, it has no effect.\n" \
"  -O, --output=<FILE>      Path to write output to. The default is\n" \
"                           /tmp/termtask/out. If used with -s, it has no effect.\n" \
"\n" \
"Formatting\n" \
"  -f, --format=<FMT>       Format string for window output. This is equivalent to\n" \
"                           --set WINDOW <FMT>. Defaults to \"%w\\n\"\n"\
"  -w, --workspace=<FMT>    Format string for desktop output. This is equivalent to\n" \
"                           --set DESKTOP <FMT>. Defaults to \"%t\\n\"\n\n"\
"  -S, --sector=<FMT>       Format string for sector output. This is equivalent to\n" \
"                           --set SECTOR <FMT>. Defaults to \"%s\\n%c\\4\"\n"\
"  -t, --set <SECTOR> <FMT> Sets format string for SECTOR. If SECTOR is not one of\n" \
"                           the default sectors, only global formatting will be\n" \
"                           available. For more information on sectors, refer\n" \
"                           to the manual.\n" \
"\n" \
" For more information on format strings, please refer to the manual.\n" \
"\n" \
"Debugging\n" \
"  -V, --verbose [LEVEL]    Set debug information output level: \n" \
"                             0: Output nothing.\n" \
"                             1: Output debugging information to stderr [default].\n" \
"                             2: Output debugging information and the processed.\n" \
"                                output to stderr.\n" \
"                           The LEVEL value is optional, and if left in blank,\n" \
"                           it'll be set to 2.\n" \
"\n" \
"Misc\n" \
"  -h, --help               Show this help. \n" \
"  -v, --version            Show version information. \n" \
"\n" \
"\n" \
"Author, current maintainer: Benjamin MdA <knuckvice@gmail.com>\n"\
"Website: https://github.com/knuck/termtask\n"\
"Released under the GNU General Public License.\n"\
"Copyright (C) 2012"

#define VERSION_INFO \
"termtask v" VERSION "\n"\
"Compiled with GCC " __VERSION__

#endif