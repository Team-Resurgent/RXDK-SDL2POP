#include <ctype.h>

int strcasecmp(const char* s1, const char* s2) {
	while (*s1 && *s2) {
		char c1 = tolower((unsigned char)*s1);
		char c2 = tolower((unsigned char)*s2);

		if (c1 != c2) {
			return (c1 - c2);
		}

		s1++;
		s2++;
	}

	return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncasecmp(const char* s1, const char* s2, int n) {
	while (n && *s1 && *s2) {
		char c1 = tolower((unsigned char)*s1);
		char c2 = tolower((unsigned char)*s2);

		if (c1 != c2) {
			return (c1 - c2);
		}

		s1++;
		s2++;
		n--;
	}

	if (n == 0) {
		return 0;
	}

	return (unsigned char)(*s1) - (unsigned char)(*s2);
}