#include "util.h"

char *Util_AppendStr(char *p, const char *s) {
    while (*s) *p++ = *s++;
    *p = 0;
    return p;
}

char *Util_AppendNum(char *p, uint16_t n) {
    char tmp[6];
    uint8_t i = 0;
    if (n == 0) {
        *p++ = '0';
    } else {
        while (n > 0) { tmp[i++] = (char)('0' + (n % 10)); n /= 10; }
        while (i > 0) *p++ = tmp[--i];
    }
    *p = 0;
    return p;
}
