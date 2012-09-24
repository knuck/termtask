#ifndef _IO_H
	#define _IO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

bool create_pipe(char* path);
bool delete_pipe(char* path);
FILE* get_out_file();
void release_out_file();

#endif