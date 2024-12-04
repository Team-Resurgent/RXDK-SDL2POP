#include <stdio.h>

size_t strnlen(const char* str, size_t max_len) {
	size_t len = 0;

	while (len < max_len && str[len] != '\0') {
		len++;
	}

	return len;
}