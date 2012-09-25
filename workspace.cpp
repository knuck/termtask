#include "workspace.h"
#include "helpers.h"
#include "x11.h"
#include <vector>

/*		virtual desktops		*/

void update_desktop_names() {
	unsigned long nitems;
	char* desk_names = (char*)get_xprop(root_win,n_atoms[deskname_atom],n_atoms[utf8Atom],&nitems);
	std::vector<std::string> converted_vec;
    char_array_list_to_vector(desk_names,converted_vec, nitems);
    int i = 0;
    while (converted_vec.size() > 0) {
    	tbar.workspaces.push_back({converted_vec.at(0),i});
    	converted_vec.erase(converted_vec.begin());
    	i++;
    }
}

void update_desktop_num() {
	unsigned char* ptr = get_xprop(root_win,n_atoms[desknum_atom],XA_CARDINAL,NULL);
	tbar.root_data.num_desktops = (int)*ptr;
	XFree(ptr);
}

void update_current_desktop() {
	unsigned char* ptr = get_xprop(root_win,n_atoms[curdesk_atom],XA_CARDINAL,NULL);
	tbar.root_data.current_desk = (int)*ptr;
	XFree(ptr);
}

void update_desktop_data() {
	update_current_desktop();
	update_desktop_num();
	update_desktop_names();
}