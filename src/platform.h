#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdio.h>
#include <stdarg.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint64_t read_tsc(void)
{
   uint64_t ts;

   __asm__ volatile(".byte 0x0f,0x31" : "=A" (ts));
   return ts;
}


static inline uint64_t cycles_elapsed(uint64_t stop_tsc, uint64_t start_tsc)
{
   return (stop_tsc - start_tsc);
}


static inline void d_printf(const char *format, ...)
{

#ifdef PRINT_DEBUG
	va_list ap;
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
#endif

}

static inline void d_error(const char *format, ...)
{
#ifdef ERROR_DEBUG
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
#endif
}

static inline void d_log(const char *format, ...)
{
#ifdef LOG_DEBUG
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
#endif
}

/* swap two vectors */
static inline void x_swap(symbol_t *a, symbol_t *b, uint32_t size)
{

}


#ifdef __cplusplus
}
#endif


#endif
