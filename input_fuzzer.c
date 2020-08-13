#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "dynamicarray.h"

int get_corpus(char *dir_c, Dynarr **arr) {
	DIR *d;
	struct dirent *dir;
	d = opendir(dir_c);
	if (d) {
		char rel_path[PATH_MAX + 1];
		while ((dir = readdir(d)) != NULL) {
			//printf("%s\n", dir->d_name);
			//memset(rel_path, 0, sizeof(rel_path));
			strcpy(rel_path, dir_c);
			strcat(rel_path, "/");
			strcat(rel_path, dir->d_name);
			dynarr_append(arr, dir->d_name);
			//printf("%s\n", rel_path);
		}
		closedir(d);
	}
	return 0;
}

void *fuzz(void *void_args) {
	
}

int main(int argc, char *argv[]) {
	if (argc == 1) {
		printf("ERROR:Provide args\n");
		exit(0);
	}

	char *target       = argv[1];
	char *target_flags = argv[2];
	char *corpus_dir   = argv[3];

	printf("Target: %s\n", target);
	printf("Target flags: %s\n", target_flags);
	printf("Corpus: %s\n", corpus_dir);

	Dynarr *corpus = dynarr_create(10);

	get_corpus(corpus_dir, &corpus);

	for (int i = 0; i < corpus->pos; i++) {
		char command[PATH_MAX+1];
		strcpy(command, target);
		strcat(command, " ");
		strcat(command, target_flags);
		strcat(command, " ");
		strcat(command, corpus_dir);
		strcat(command, "/");
		strcat(command, corpus->values[i]);
		strcat(command, " > /dev/null 2>&1");

		//printf("Running %s\n", corpus->values[i]);
		int status = system(command);
		printf("%d\t%s\t%d\n", i, command, status);
		if (status == 11 || status == -11)
			break;
	}
}
