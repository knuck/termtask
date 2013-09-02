#ifndef _FORMATTING_H
	#define _FORMATTING_h
#include <string>
#include <map>
#include <functional>
#include <algorithm>
#include "types.h"
#include "helpers.h"

extern std::map<std::string,std::function<std::string(task&)>> win_fmts;
extern std::map<std::string,std::function<std::string(workspace&)>> work_fmts;
extern std::map<std::string,std::function<std::string()>> gen_fmts;
extern std::map<std::string,std::function<std::string(sector_type&)>> sec_fmts;
extern std::map<std::string, std::function<std::vector<task>(std::vector<task>&)>> grp_fmts;

template <typename T> bool eval_format_string(std::string targ_str, std::map<std::string, std::function<std::string(T&)>> allowed_fmt) {
	bool is_in_fmt = false;
	std::string current_format = "";
	for (auto it = targ_str.begin(); it != targ_str.end(); it++) {
		if (is_in_fmt) {
			if (current_format[0] == *it) {
				current_format += "";
			} else {
				//printf("searching for %s\n", (current_format+(*it)).c_str());
				if (allowed_fmt.find(current_format+(*it)) != allowed_fmt.end()) {
					current_format = "";
				} else {
				//	printf("failed\n");
					return false;
				}
			}
			is_in_fmt = false;
		} else {
			if (	find_if(begin(allowed_fmt),
							end(allowed_fmt),
							[it] (std::pair<const std::string, std::function<std::string(T&)>>& sit)
								-> bool {
									return sit.first[0] == *it;
								}
							) != end(allowed_fmt)
				) {
					is_in_fmt = true;
					current_format += *it;
			}
		}
	}
	return true;
}

extern std::string generic_format_string(std::string targ_str, std::map<std::string, std::function<std::string()>>& format_types);


template <typename T> std::string generic_format_string(std::string targ_str, std::map<std::string, std::function<std::string(T&)>>& format_types, T& data) {
	bool is_in_fmt = false;
	std::string current_format = "";
	std::string out_str;
	for (auto it = targ_str.begin(); it != targ_str.end(); it++) {
		if (is_in_fmt) {
			if (current_format[0] == *it) {
				current_format = "";
				out_str+=*it;
			} else {
				out_str += format_types[current_format+(*it)](data);
				current_format = "";
			}
			is_in_fmt = false;
		} else {
			if (format_types.end()!=find_if(format_types.begin(),format_types.end(),[it] (std::pair<const std::string, std::function<std::string(T&)>>& sit) -> bool {
				return sit.first[0] == *it;
			})) {
				is_in_fmt = true;
				current_format += *it;
			} else {
				out_str += *it;
			}
		}
	}
	return out_str;
}

template <typename T> std::string generic_build_string(std::string targ_str, std::map<std::string, std::function<std::string(T&)>>& format_types, std::vector<T>& data) {
	std::string out_str;
	for (auto it = data.begin(); it != data.end(); it++) {
		out_str += generic_format_string(targ_str,format_types,*it);
	}
	return out_str;
}

#endif
