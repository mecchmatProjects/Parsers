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
  
  Inpired by the verson of JSON parsing lib:
  https://github.com/skeeto/pdjson

*/

#ifndef __JSON_KV_PARSER_H__
#define __JSON_KV_PARSER_H__


#ifdef __cplusplus
extern "C" {
#else
#endif /* __cplusplus */

#include "utilits.h"


/*
 * struct to keep allocator in function form
 * */
typedef struct kvp_allocator {
    void *(*malloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
}kvp_allocator;

/* 
 * function type for communication with streams
 * */
typedef int (*json_user_io)(void *user);


/////////////////////////////////////////


// types of state machine
enum kvp_state_machine_type {
    KV_ERROR = 1, KV_START,
    KV_READKEY, KV_READSEMI,
    KV_READVAL, KV_READEND,   
    KV_END,
};

// type of JSON to be parsed
enum kvp_json_type{
    
    JSON_ERROR = 1,
//  JSON_ARRAY, JSON_OBJECT,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
    JSON_END,
};

/*
 * JSON sorce struct:
 * source is either FILE / text buffer/ user input;
 * keeps the postion in source.
 * Methods:
 * - get : receives next char
 * - peek: set current char
 * */
typedef struct json_source {
    int (*get)(struct json_source *);
    int (*peek)(struct json_source *);
    
    size_t position;
    
    union {
        struct {
            FILE *stream;
        } stream;
        struct {
            const char *buffer;
            size_t length;
        } buffer;
        struct {
            void *ptr;
            json_user_io get;
            json_user_io peek;
        } user;
    } source;
}json_source;


/*
 * Iterator to keep current element of JSON
 * Keeps:
 * - information of read JSON KVP list
 * - data and type of value
 * - source, allocator, error state
 * */
typedef struct kvp_iterator{

    size_t lineno; /// current line
    size_t ntokens; /// current token number
    
    int figure_brackets; /// open ot closed bracket
    int comas; // number of comas
    int semis; // number of semicolons
    
    // data is stored in buffer
    struct {
        char *string;
        size_t string_fill;
        size_t string_size;        
    } data; /// data
    
   bool isKey; /// is is key or value
   
   enum kvp_json_type type; /// the type of the data
  
   unsigned flags;
      
   struct json_source source; /// source
   struct kvp_allocator alloc; /// allocator
   char errmsg[LEN_ERROR_MSG]; // error message
}kvp_iterator;




/*
 * read KVP from buffer mode 
 * */
void kvp_open_buffer(kvp_iterator *json, const void *buffer, size_t size);
/*
 * read KVP from string mode 
 * */
void kvp_open_string(kvp_iterator *json, const char *string);
/*
 * read KVP from file mode
 * */
void kvp_open_stream(kvp_iterator *json, FILE *stream);

/*
 * read KVP from user cosole mode 
 * */
void kvp_open_user(kvp_iterator *json, json_user_io get, json_user_io peek, void *user);

/*
 * close KVP iterator 
 * */
void kvp_close(kvp_iterator *json);
/*
 * set KVP allocator
 * */
void kvp_set_allocator(kvp_iterator *json, kvp_allocator *a);

void kvp_set_streaming(kvp_iterator *json, bool mode);


/*
 *  go to the next JSON value
 * */
enum kvp_json_type kvp_next(kvp_iterator *json);
//enum kv_state_machine_type json_peek_pair(kvp_iterator *json);

/*
 * initialize iterator
 * */
void init_iterator(kvp_iterator *json);

/*
 * reset iterator
 * */
void kvp_reset_iterator(kvp_iterator *json);

/*
 * return buffer of data
 * */
const char * kvp_get_string(kvp_iterator *json, size_t *length);


size_t kvp_save_string(kvp_iterator *json, char *buf);
/*
 * return double number of data
 * */
double kvp_get_number(kvp_iterator *json);

/*
 * return int number of data
 * */
int kvp_get_int(kvp_iterator *json);

/*
 * return arbitrary value as char*
 * */
bool kvp_get_value(kvp_iterator *json, char* buf, size_t *length);


/*
 * get number of line
 * */
size_t kvp_get_lineno(kvp_iterator *json);
/*
 * get position
 * */
size_t kvp_get_position(kvp_iterator *json);
/*
 * get type of the data
 * */
enum kvp_json_type kvp_get_type(kvp_iterator *json);

/*
 * get error
 * */
const char *kvp_get_error(kvp_iterator *json);



#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif //__JSON_KV_PARSER_H__
