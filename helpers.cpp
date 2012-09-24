#include <string.h>
#include "helpers.h"
#include "types.h"

/*		c++11 std::string literal		*/
std::string operator "" s (const char* p, size_t) {
	return std::string(p);
}

void char_array_list_to_vector(char* list, std::vector<std::string>& out, unsigned long nitems) {
	out.clear();
	std::string namebuf;
    int i = 0;
    while (i < nitems) {
    	namebuf = list+i;
    	out.push_back(namebuf);
    	i += strlen(list+i)+1;
    }
}