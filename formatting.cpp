#include "formatting.h"

/*		non-template version (for constant non data-dependent formatting)		*/

std::string generic_format_string(std::string targ_str, std::map<std::string, std::function<std::string()>>& format_types) {
	bool is_in_fmt = false;
	std::string current_format = "";
	std::string out_str;
	for (auto it = targ_str.begin(); it != targ_str.end(); it++) {
		if (is_in_fmt) {
			if (current_format[0] == *it) {
				current_format = "";
				out_str+=*it;
			} else {
				out_str += format_types[current_format+(*it)]();
				current_format = "";
			}
			is_in_fmt = false;
		} else {
			if (format_types.end()!=find_if(format_types.begin(),format_types.end(),[it] (std::pair<const std::string, std::function<std::string()>>& sit) -> bool {
					return sit.first[0] == *it; })) {
				is_in_fmt = true;
				current_format += *it;
			} else {
				out_str += *it;
			}
		}
	}
	return out_str;
}

/*		lambdas for formatting		*/

/*		window formatting		*/

std::map<std::string,std::function<std::string(task&)>> win_fmts = {
	{"%w",[](task &t) -> std::string {
						char buf[256];
						sprintf(buf,"%.*s",tbar.settings.max_title_size,t.title.c_str());
						return buf;
					   }
	},
	{"%d",[](task &t) -> std::string {
						char buf[20];
						sprintf(buf,"%ld",(signed long *)t.desktop);
						return buf;
					   }
	},
	{"%p",[](task &t) -> std::string {
						char buf[20];
						sprintf(buf,"0x02%x",(signed long *)t.pid);
						return buf;
					   }
	},
	{"%P",[](task &t) -> std::string {
						char buf[20];
						sprintf(buf,"0x02%X",(signed long *)t.pid);
						return buf;
					   }
	},
	{"%c",[](task &t) -> std::string {
						char buf[256];
						sprintf(buf,"%s", t.command.c_str());
						return buf;
					   }
	},
	{"%x",[](task &t) -> std::string {
						char buf[256];
						sprintf(buf,"0x02%x", t.wid);
						return buf;
					   }
	},
	{"%X",[](task &t) -> std::string {
						char buf[256];
						sprintf(buf,"0x02%X", t.wid);
						return buf;
	}}
};

/*		workspace formatting		*/

std::map<std::string,std::function<std::string(workspace&)>> work_fmts = {
	{"%t",[](workspace &wp) -> std::string {
						return wp.title;
					   }
	},
	{"%d",[](workspace &wp) -> std::string {
						char buf[20];
						sprintf(buf,"%ld", (signed long *)wp.pos);
						return buf;
	}}
};

/*		general formatting		*/

std::map<std::string,std::function<std::string()>> gen_fmts = {
	{"%D",[]() -> std::string {
						char buf[20];
						sprintf(buf,"%ld",tbar.root_data.num_desktops);
						return buf;
					   }
	},
	{"%n",[]() -> std::string {
						char buf[20];
						sprintf(buf,"%d",tbar.ordered_tasks.size());
						return buf;
					   }
	},
	{"%c",[]() -> std::string {
						char buf[20];
						sprintf(buf,"%ld",tbar.root_data.current_desk);
						return buf;
	}}
};

/*		sector formatting		*/

std::map<std::string,std::function<std::string(sector_type&)>> sec_fmts = {
	{"%s",[](sector_type& sec) -> std::string {
		return sec.first;
	}},
	{"%c",[](sector_type& sec) -> std::string {
		if (sec.first == "WINDOWS") {
			return generic_build_string(sec.second, win_fmts,tbar.ordered_tasks);
		} else if (sec.first == "DESKTOPS") {
			return generic_build_string(sec.second, work_fmts,tbar.workspaces);
		} else {
			return generic_format_string(sec.second, gen_fmts);
		}
	}}
};