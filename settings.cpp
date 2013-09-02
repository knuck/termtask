#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"
#include "formatting.h"
#include "types.h"
#include "fail.h"

extern taskbar tbar;
extern std::map<std::string,std::string> sector_list;
void set_sector_fmt(std::string secname, std::string fmt) {
	tbar.settings.sector_list[secname] = fmt;
}

void set_in_file(char* path) {
	strcpy(tbar.settings.in_file, path);
}

void set_out_file(char* path) {
	strcpy(tbar.settings.out_file, path);
}

void set_in_file(const char* path) {
	strcpy(tbar.settings.in_file, path);
}

void set_out_file(const char* path) {
	strcpy(tbar.settings.out_file, path);
}

void set_sector_file(char* path) {
	strcpy(tbar.settings.sector_file, path);
}

void set_sector_file(const char* path) {
	strcpy(tbar.settings.sector_file, path);
}

void parse_args(int argc, char* argv[]) {
	static option long_options[] = {
		{"config",required_argument,NULL,'c'},
		{"deaf",no_argument,NULL,'d'},
		{"interactive",no_argument,NULL,'i'},
		{"pipes",no_argument,NULL,'p'},
		{"input",required_argument,NULL,'I'},
		{"output",required_argument,NULL,'O'},
		{"format",required_argument,NULL,'f'},
		{"foreground",required_argument,NULL,'F'},
		{"workspace",required_argument,NULL,'w'},
		{"verbose",optional_argument,NULL,'V'},
		{"stdout",no_argument,NULL,'s'},
		{"sector",required_argument,NULL,'S'},
		{"set",required_argument,NULL,'t'},
		{"help",no_argument,NULL,'h'},
		{"version",no_argument,NULL,'v'},
		{0,0,0}
	};
	int c;
	int option_index;
	std::string cur_sec_name = "";
	while ((c = getopt_long(argc, argv, "-dipshvFS:I:O:t:f:w:c:V::",long_options,&option_index)) != -1) {
		switch (c) {
			case 1: {
				if (cur_sec_name != "") {
					//if (eval_format_string(optarg,"Dnc","%") == false) fail("Invalid format string.");
					set_sector_fmt(cur_sec_name,optarg);
					cur_sec_name = "";
				}
				break;
			}
			case 'd': {
				tbar.settings.deaf = true;
				break;
			}
			case 'i': {
				tbar.settings.interactive = true;
				break;
			}
			case 'p': {
				tbar.settings.force_pipes = true;
				break;
			}
			case 's': {
				tbar.settings.stdout = true;
				//set_out_file(stdout);
				break;
			}
			case 't': {
				cur_sec_name = optarg;
				break;
			}
			case 'I': {
				set_in_file(optarg);
				break;
			}
			case 'O': {
				set_out_file(optarg);
				break;
			}
			case 'f': {
				if (eval_format_string(optarg,win_fmts) == false) fail("Invalid format string.");
				set_sector_fmt("WINDOWS",optarg);
				
				break;
			}
			case 'w': {
				if (eval_format_string(optarg,work_fmts) == false) fail("Invalid workspace format string.");
				set_sector_fmt("DESKTOPS", optarg);
				break;
			}
			case 'S': {
				if (eval_format_string(optarg,sec_fmts) == false) fail("Invalid sector format string.");
				tbar.settings.sector_fmt = optarg;
				break;
			}
			case 'V': {
				if (optarg) {
					tbar.settings.verbose_level = atoi(optarg);
				} else {
					tbar.settings.verbose_level = 2;
				}
				break;
			}
			case 'h': {
				printf("%s\n", HELP);
				clean_up();
				exit(0);
				break;
			}
			case 'v': {
				printf("%s\n", VERSION_INFO);
				clean_up();
				exit(0);
				break;
			}
		}
	}
}