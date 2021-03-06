#include "io.h"
#include "types.h"
#include "helpers.h"

/*		pipe handling		*/

bool create_pipe(char* path) {
	if (mkfifo(path,0666) != 0) {
		if (errno != EEXIST) return false;
	}
	return true;
}

bool delete_pipe(char* path) {
	if (unlink(path) != 0) {
		return false;
	}
	return true;
}

/*		io management		*/

FILE* get_out_file() {
	if (tbar.settings.stdout) {
		return stdout;
	}
	tbar.settings.out_file_pointer = fopen(tbar.settings.out_file,"w");
	return tbar.settings.out_file_pointer;
}

void release_out_file() {
	if (!tbar.settings.stdout) {
		fclose(tbar.settings.out_file_pointer);
	}
}

FILE* get_sector_file() {
	tbar.settings.sec_file_pointer = fopen(tbar.settings.sector_file,"w");
	return tbar.settings.sec_file_pointer;
}

void release_sector_file() {
	fclose(tbar.settings.sec_file_pointer);
}