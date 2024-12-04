#ifndef	_SYS_STDINT_H_
#define _SYS_STDINT_H_

#define _STDINT_H_

#include <xtl.h>

typedef signed char int8_t;
typedef short int16_t;
typedef long int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
typedef long long intmax_t;

typedef uint32_t uintptr_t;
typedef int32_t intptr_t;

#define _INTPTR_T_DEFINED
#define _UINTPTR_T_DEFINED

#define	INT8_MIN		(-0x7f - 1)
#define	INT16_MIN		(-0x7fff - 1)
#define	INT32_MIN		(-0x7fffffff - 1)
#define	INT64_MIN		(-0x7fffffffffffffffLL - 1)

#define	INT8_MAX		0x7f
#define	INT16_MAX		0x7fff
#define	INT32_MAX		0x7fffffff
#define	INT64_MAX		0x7fffffffffffffffLL

#define	UINT8_MAX		0xff
#define	UINT16_MAX		0xffff
#define	UINT32_MAX		0xffffffffU
#define	UINT64_MAX		0xffffffffffffffffULL

#endif /* _SYS_STDINT_H_ */