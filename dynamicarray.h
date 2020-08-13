#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	char **values;
	int size;
	int pos;
} Dynarr;

Dynarr* dynarr_create(int size);

Dynarr *dynarr_grow(Dynarr *arr);

void dynarr_append(Dynarr **arr, char *value);

#endif //DYNAMICARRAY_H
