/*
The MIT License (MIT)

Copyright (c) 2022, Viktor Borodin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Based upon modified code of
 *  https://github.com/Jhuster/TLV
 */
#ifndef _TLV_WORK_H_
#define _TLV_WORK_H_

#ifdef __cplusplus
extern "C" {
#else
#endif /* __cplusplus */

#include "key_list.h"

#include <inttypes.h>
#include <stdlib.h>

#define BAD_OUT_FILE -8
#define BAD_FILE_WRITE -9
#define BAD_IN_FILE -10
#define BAD_FILE_READ -11

#define NUMBER_TLV 1
#define STRING_TLV 2
#define BOOL_TLV 3

typedef uint8_t TYPE_TYPE;
typedef uint8_t TYPE_LENGTH;
typedef char TYPE_VALUE;

typedef struct _tlv {
    TYPE_TYPE type;
    TYPE_LENGTH length;
    TYPE_VALUE* value;
} tlv_t;

int tlv_write_file(TYPE_TYPE type, TYPE_LENGTH length, void* value, FILE* fp);

int tlv_read_object_file(TYPE_TYPE* type, TYPE_LENGTH* length, void* value, FILE* fp);

typedef struct _tlv_box {
    key_list_t* m_list;
    unsigned char* m_serialized_buffer;
    int m_serialized_bytes;
} tlv_box_t;

tlv_box_t* tlv_box_create();
tlv_box_t* tlv_box_parse(unsigned char* buffer, TYPE_LENGTH buffersize);
int tlv_box_destroy(tlv_box_t* box);

unsigned char* tlv_box_get_buffer(tlv_box_t* box);
int tlv_box_get_size(tlv_box_t* box);

int tlv_box_put_char(tlv_box_t* box, TYPE_TYPE type, char value);
int tlv_box_put_short(tlv_box_t* box, TYPE_TYPE type, short value);
int tlv_box_put_int(tlv_box_t* box, TYPE_TYPE type, int value);
int tlv_box_put_long(tlv_box_t* box, TYPE_TYPE type, long value);
int tlv_box_put_longlong(tlv_box_t* box, TYPE_TYPE type, long long value);
int tlv_box_put_float(tlv_box_t* box, TYPE_TYPE type, float value);
int tlv_box_put_double(tlv_box_t* box, TYPE_TYPE type, double value);
int tlv_box_put_string(tlv_box_t* box, TYPE_TYPE type, char* value);
int tlv_box_put_bytes(tlv_box_t* box, TYPE_TYPE type, unsigned char* value, TYPE_LENGTH length);
int tlv_box_put_object(tlv_box_t* box, TYPE_TYPE type, tlv_box_t* object);
int tlv_box_serialize(tlv_box_t* box);

int tlv_box_get_char(tlv_box_t* box, TYPE_TYPE type, char* value);
int tlv_box_get_short(tlv_box_t* box, TYPE_TYPE type, short* value);
int tlv_box_get_int(tlv_box_t* box, TYPE_TYPE type, int* value);
int tlv_box_get_long(tlv_box_t* box, TYPE_TYPE type, long* value);
int tlv_box_get_longlong(tlv_box_t* box, TYPE_TYPE type, long long* value);
int tlv_box_get_float(tlv_box_t* box, TYPE_TYPE type, float* value);
int tlv_box_get_double(tlv_box_t* box, TYPE_TYPE type, double* value);
int tlv_box_get_string(tlv_box_t* box, TYPE_TYPE type, char* value, TYPE_LENGTH* length);
int tlv_box_get_bytes(tlv_box_t* box, TYPE_TYPE type, unsigned char* value, TYPE_LENGTH* length);
int tlv_box_get_bytes_ptr(tlv_box_t* box, TYPE_TYPE type, unsigned char** value, TYPE_LENGTH* length);
int tlv_box_get_object(tlv_box_t* box, TYPE_TYPE type, tlv_box_t** object);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif //_TLV_WORK_H_
