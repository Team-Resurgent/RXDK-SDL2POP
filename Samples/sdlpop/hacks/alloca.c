#include <stdlib.h>
#include <stdio.h>
#
static size_t block_length;
static void* block = NULL;

// Pretty hacky. Cannot do two alloca within the same stack frame. Doesnt seem like sdlpop does this
void* alloca(size_t size) {
	if (block == NULL) {
		block = malloc(size);
		block_length = size;
	}

	else if (block_length < size) {
		block = realloc(block, size);
		block_length = size;
	}

	return block;
}