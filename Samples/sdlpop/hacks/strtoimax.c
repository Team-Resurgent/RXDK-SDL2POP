#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdint.h>

#define INTMAX_MAX LLONG_MAX
#define INTMAX_MIN LLONG_MIN

intmax_t strtoimax(const char* str, char** endptr, int base) {
	const char* ptr = str;
	intmax_t result = 0;
	int sign = 1;

	// Skip leading whitespace
	while (isspace((unsigned char)*ptr)) {
		ptr++;
	}

	// Handle optional '+' or '-' sign
	if (*ptr == '+' || *ptr == '-') {
		if (*ptr == '-') {
			sign = -1;
		}
		ptr++;
	}

	// Determine base if it's 0
	if (base == 0) {
		if (*ptr == '0') {
			if (*(ptr + 1) == 'x' || *(ptr + 1) == 'X') {
				base = 16;
				ptr += 2;
			}
			else {
				base = 8;
				ptr++;
			}
		}
		else {
			base = 10;
		}
	}

	// Validate base
	if (base < 2 || base > 36) {
		if (endptr) {
			*endptr = (char*)str;
		}
		return 0;
	}

	// Convert string to integer
	while (*ptr) {
		int digit;

		if (*ptr >= '0' && *ptr <= '9') {
			digit = *ptr - '0';
		}
		else if (*ptr >= 'a' && *ptr <= 'z') {
			digit = *ptr - 'a' + 10;
		}
		else if (*ptr >= 'A' && *ptr <= 'Z') {
			digit = *ptr - 'A' + 10;
		}
		else {
			break;
		}

		if (digit >= base) {
			break;
		}

		// Check for overflow
		if (result > (INTMAX_MAX - digit) / base) {
			result = (sign == 1) ? INTMAX_MAX : INTMAX_MIN;
			if (endptr) {
				*endptr = (char*)ptr;
			}
			return result;
		}

		result = result * base + digit;
		ptr++;
	}

	// Set endptr to the first invalid character
	if (endptr) {
		*endptr = (char*)ptr;
	}

	return sign * result;
}