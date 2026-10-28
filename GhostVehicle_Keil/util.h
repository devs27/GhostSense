#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>

/* Tiny string builders (no printf, no heap). Both keep the buffer NUL-terminated
   and return a pointer to the terminator so calls can be chained. */
char *Util_AppendStr(char *p, const char *s);
char *Util_AppendNum(char *p, uint16_t n);

#endif
