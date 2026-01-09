/* esp32_values.h - Replacement for missing values.h on ESP32 */

#ifndef _ESP32_VALUES_H
#define _ESP32_VALUES_H

#include <limits.h>
#include <float.h>
#include <stdint.h>

/* Character bit sizes */
#define CHARBITS    8
#define SHORTBITS   16
#define INTBITS     32
#define LONGBITS    32
#define PTRBITS     32
#define DOUBLEBITS  64
#define FLOATBITS   32

/* Integer type max/min values */
#define MAXCHAR     CHAR_MAX
#define MAXSHORT    SHRT_MAX
#define MAXINT      INT_MAX
#define MAXLONG     LONG_MAX

#define MINCHAR     CHAR_MIN
#define MINSHORT    SHRT_MIN
#define MININT      INT_MIN
#define MINLONG     LONG_MIN

#define MAXUCHAR    UCHAR_MAX
#define MAXUSHORT   USHRT_MAX
#define MAXUINT     UINT_MAX
#define MAXULONG    ULONG_MAX

/* Floating point limits */
#define MAXFLOAT    FLT_MAX
#define MINFLOAT    FLT_MIN
#define MAXDOUBLE   DBL_MAX
#define MINDOUBLE   DBL_MIN

/* Bit manipulation */
#define BITSPERBYTE 8
#define HIBITS      (~(~(unsigned)0 >> 1))
#define HIBITL      (~(~(unsigned long)0 >> 1))

#endif /* _ESP32_VALUES_H */
