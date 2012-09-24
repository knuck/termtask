#ifndef _HELPERS_H
	#define _HELPERS_H


#include <string>
#include <vector>

std::string operator "" s (const char* p, size_t);
void char_array_list_to_vector(char* list, std::vector<std::string>& out, unsigned long nitems);



#endif