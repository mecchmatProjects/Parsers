/*
 *  Helper functions, utility and codes file
 *
 * */

#ifndef __UTILITS_H_
#define __UTILITS_H_

/* library for I/O routines        */
#include <stdio.h>

/* library for memory routines     */
#include <stdlib.h>

/* for isdigit function*/
#include <ctype.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#else
// use boolean type or define it
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#include <stdbool.h>
#else
#ifndef bool
#define bool int
#define true 1
#define false 0
#endif /* bool */
#endif /* __STDC_VERSION__ */
#endif /* __cplusplus */

// Definitions
#define EXIT_NO_ERRORS 0
#define EXIT_WRONG_ARG_COUNT 1 // wrong pragramm call arguments
#define EXIT_BAD_FILE_NAME 2   // Programme failed to open a file stream
#define EXIT_BAD_HASH_TABLE 3  // Programme failed on hash creation
#define EXIT_BAD_MALLOC 4      // Malloc failed
#define EXIT_BAD_OUTPUT_FILE 5 //   Programme failed during output
#define EXIT_JSON_ERROR 6      // Programme failed on max gray value

//#define EXIT_BAD_READ 8 //Programme failed when reading in data
//#define EXIT_BAD_LAYOUT 10 // Layout file for assembly went wrong
//#define EXIT_MISC 100 // Any other error that is detected.

#define JSON_FLAG_ERROR (1u << 0)
#define JSON_FLAG_STREAMING (1u << 1)

#if defined(_MSC_VER) && (_MSC_VER < 1900)

#define json_error(json, format, ...)                                                    \
    if(!(json->flags & JSON_FLAG_ERROR)) {                                               \
        json->flags |= JSON_FLAG_ERROR;                                                  \
        _snprintf_s(json->errmsg, sizeof(json->errmsg), _TRUNCATE, format, __VA_ARGS__); \
    }

#else

#define json_error(json, format, ...)                                      \
    if(!(json->flags & JSON_FLAG_ERROR)) {                                 \
        json->flags |= JSON_FLAG_ERROR;                                    \
        snprintf(json->errmsg, sizeof(json->errmsg), format, __VA_ARGS__); \
    }

#endif /* _MSC_VER */

/* See also PDJSON_STACK_MAX below. */
/*#ifndef PDJSON_STACK_INC
#  define PDJSON_STACK_INC 4
#endif
*/

#define LEN_ERROR_MSG 128

static int char_needs_escaping(int c)
{
    if((c >= 0) && (c < 0x20 || c == 0x22 || c == 0x5c)) {
        return 1;
    }

    return 0;
}

static int utf8_seq_length(char byte)
{
    unsigned char u = (unsigned char)byte;
    if(u < 0x80)
        return 1;

    if(0x80 <= u && u <= 0xBF) {
        // second, third or fourth byte of a multi-byte
        // sequence, i.e. a "continuation byte"
        return 0;
    } else if(u == 0xC0 || u == 0xC1) {
        // overlong encoding of an ASCII byte
        return 0;
    } else if(0xC2 <= u && u <= 0xDF) {
        // 2-byte sequence
        return 2;
    } else if(0xE0 <= u && u <= 0xEF) {
        // 3-byte sequence
        return 3;
    } else if(0xF0 <= u && u <= 0xF4) {
        // 4-byte sequence
        return 4;
    } else {
        // u >= 0xF5
        // Restricted (start of 4-, 5- or 6-byte sequence) or invalid UTF-8
        return 0;
    }
}

static int is_legal_utf8(const unsigned char* bytes, int length)
{
    if(0 == bytes || 0 == length)
        return 0;

    unsigned char a;
    const unsigned char* srcptr = bytes + length;
    switch(length) {
    default:
        return 0;
        // Everything else falls through when true.
    case 4:
        if((a = (*--srcptr)) < 0x80 || a > 0xBF)
            return 0;
        /* FALLTHRU */
    case 3:
        if((a = (*--srcptr)) < 0x80 || a > 0xBF)
            return 0;
        /* FALLTHRU */
    case 2:
        a = (*--srcptr);
        switch(*bytes) {
        case 0xE0:
            if(a < 0xA0 || a > 0xBF)
                return 0;
            break;
        case 0xED:
            if(a < 0x80 || a > 0x9F)
                return 0;
            break;
        case 0xF0:
            if(a < 0x90 || a > 0xBF)
                return 0;
            break;
        case 0xF4:
            if(a < 0x80 || a > 0x8F)
                return 0;
            break;
        default:
            if(a < 0x80 || a > 0xBF)
                return 0;
            break;
        }
        /* FALLTHRU */
    case 1:
        if(*bytes >= 0x80 && *bytes < 0xC2)
            return 0;
    }
    return *bytes <= 0xF4;
}

static int getxdigit(int c)
{
    if(!isxdigit(c)) {
        return -1;
    }
    int d;
    char buf[4];
    snprintf(buf, 4, "0x%c", (char)c);
    sscanf(buf, "%x", &d);
    return d;
}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif // __UTILITS_H_
