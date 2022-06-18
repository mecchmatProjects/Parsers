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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "kvp_parser.h"


static int buffer_peek(struct json_source *source)
{
    if (source->position < source->source.buffer.length)
        return source->source.buffer.buffer[source->position];
    else
        return EOF;
}

static int buffer_get(struct json_source *source)
{
    int c = source->peek(source);
    source->position++;
    return c;
}

static int stream_get(struct json_source *source)
{
    source->position++;
    return fgetc(source->source.stream.stream);
}

static int stream_peek(struct json_source *source)
{
    int c = fgetc(source->source.stream.stream);
    ungetc(c, source->source.stream.stream);
    return c;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////


void kvp_open_buffer(kvp_iterator *json, const void *buffer, size_t size)
{
    init_iterator(json);
    json->source.get = buffer_get;
    json->source.peek = buffer_peek;
    json->source.source.buffer.buffer = (const char *)buffer;
    json->source.source.buffer.length = size;
}

void kvp_open_string(kvp_iterator *json, const char *string)
{
    kvp_open_buffer(json, string, strlen(string));
}

void kvp_open_stream(kvp_iterator *json, FILE * stream)
{
    init_iterator(json);
    json->source.get = stream_get;
    json->source.peek = stream_peek;
    json->source.source.stream.stream = stream;
}


void init_iterator(kvp_iterator *json)
{
    json->lineno = 0;
    json->flags = JSON_FLAG_STREAMING;
    
    json->errmsg[0] = '\0';
    
    json->data.string = NULL;
    json->data.string_size = 0;
    json->data.string_fill = 0;
    json->source.position = 0;
    
    json->isKey = true;

    json->alloc.malloc = malloc;
    json->alloc.realloc = realloc;
    json->alloc.free = free;
    
}



static enum kvp_json_type
is_match_pair(kvp_iterator *json, const char *pattern, enum kvp_json_type type)
{
    int c;
    for (const char *p = pattern; *p; p++) {
        if (*p != (c = json->source.get(&json->source))) {
            json_error(json, "expected '%c' instead of byte '%c'", *p, c);
            return JSON_ERROR;
        }
    }
    return type;
}

static int pushchar(kvp_iterator *json, int c)
{
    if (json->data.string_fill == json->data.string_size) {
        size_t size = json->data.string_size * 2;
        char *buffer = (char *)json->alloc.realloc(json->data.string, size);
        if (buffer == NULL) {
            json_error(json, "%s", "out of memory");
            return -1;
        } else {
            json->data.string_size = size;
            json->data.string = buffer;
        }
    }
    json->data.string[json->data.string_fill++] = c;
    return 0;
}

static int init_string(kvp_iterator *json)
{
    json->data.string_fill = 0;
    if (json->data.string == NULL) {
        json->data.string_size = 1024;
        json->data.string = (char *)json->alloc.malloc(json->data.string_size);
        if (json->data.string == NULL) {
            json_error(json, "%s", "out of memory");
            return -1;
        }
    }
    json->data.string[0] = '\0';
    return 0;
}

static int encode_utf8(kvp_iterator *json, unsigned long c)
{
    if (c < 0x80UL) {
        return pushchar(json, c);
    } else if (c < 0x0800UL) {
        return !((pushchar(json, (c >> 6 & 0x1F) | 0xC0) == 0) &&
                 (pushchar(json, (c >> 0 & 0x3F) | 0x80) == 0));
    } else if (c < 0x010000UL) {
        if (c >= 0xd800 && c <= 0xdfff) {
            json_error(json, "invalid codepoint %06lx", c);
            return -1;
        }
        return !((pushchar(json, (c >> 12 & 0x0F) | 0xE0) == 0) &&
                 (pushchar(json, (c >>  6 & 0x3F) | 0x80) == 0) &&
                 (pushchar(json, (c >>  0 & 0x3F) | 0x80) == 0));
    } else if (c < 0x110000UL) {
        return !((pushchar(json, (c >> 18 & 0x07) | 0xF0) == 0) &&
                (pushchar(json, (c >> 12 & 0x3F) | 0x80) == 0) &&
                (pushchar(json, (c >> 6  & 0x3F) | 0x80) == 0) &&
                (pushchar(json, (c >> 0  & 0x3F) | 0x80) == 0));
    } else {
        json_error(json, "unable to encode %06lx as UTF-8", c);
        return -1;
    }
}



static long
read_unicode_cp(kvp_iterator *json)
{
    long cp = 0;
    int shift = 12;

    for (size_t i = 0; i < 4; i++) {
        int c = json->source.get(&json->source);
        int hc;

        if (c == EOF) {
            json_error(json, "%s", "unterminated string literal in Unicode");
            return -1;
        } else if ((hc = getxdigit(c)) == -1) {
            json_error(json, "invalid escape Unicode byte '%c'", c);
            return -1;
        }

        cp += hc * (1 << shift);
        shift -= 4;
    }


    return cp;
}

static int read_unicode(kvp_iterator *json)
{
    long cp, h, l;

    if ((cp = read_unicode_cp(json)) == -1) {
        return -1;
    }

    if (cp >= 0xd800 && cp <= 0xdbff) {
        /* This is the high portion of a surrogate pair; we need to read the
         * lower portion to get the codepoint
         */
        h = cp;

        int c = json->source.get(&json->source);
        if (c == EOF) {
            json_error(json, "%s", "unterminated string literal in Unicode");
            return -1;
        } else if (c != '\\') {
            json_error(json, "invalid continuation for surrogate pair '%c', "
                             "expected '\\'", c);
            return -1;
        }

        c = json->source.get(&json->source);
        if (c == EOF) {
            json_error(json, "%s", "unterminated string literal in Unicode");
            return -1;
        } else if (c != 'u') {
            json_error(json, "invalid continuation for surrogate pair '%c', "
                             "expected 'u'", c);
            return -1;
        }

        if ((l = read_unicode_cp(json)) == -1) {
            return -1;
        }

        if (l < 0xdc00 || l > 0xdfff) {
            json_error(json, "surrogate pair continuation \\u%04lx out "
                             "of range (dc00-dfff)", l);
            return -1;
        }

        cp = ((h - 0xd800) * 0x400) + ((l - 0xdc00) + 0x10000);
    } else if (cp >= 0xdc00 && cp <= 0xdfff) {
            json_error(json, "dangling surrogate \\u%04lx", cp);
            return -1;
    }

    return encode_utf8(json, cp);
}

static int
read_escaped(kvp_iterator *json)
{
    int c = json->source.get(&json->source);
    if (c == EOF) {
        json_error(json, "%s", "unterminated string literal in escape");
        return -1;
    } else if (c == 'u') {
        if (read_unicode(json) != 0)
            return -1;
    } else {
        switch (c) {
        case '\\':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case '/':
        case '"':
            {
                const char *codes = "\\bfnrt/\"";
                const char *p = strchr(codes, c);
                if (pushchar(json, "\\\b\f\n\r\t/\""[p - codes]) != 0)
                    return -1;
            }
            break;
        default:
            json_error(json, "invalid escaped byte '%c'", c);
            return -1;
        }
    }
    return 0;
}




static int
read_utf8(kvp_iterator* json, int next_char)
{
    int count = utf8_seq_length(next_char);
    if (!count)
    {
        json_error(json, "%s", "invalid UTF-8 character");
        return -1;
    }

    char buffer[4];
    buffer[0] = next_char;
    int i;
    for (i = 1; i < count; ++i)
    {
        buffer[i] = json->source.get(&json->source);;
    }

    if (!is_legal_utf8((unsigned char*) buffer, count))
    {
        json_error(json, "%s", "invalid UTF-8 text");
        return -1;
    }

    for (i = 0; i < count; ++i)
    {
        if (pushchar(json, buffer[i]) != 0)
            return -1;
    }
    return 0;
}

static enum kvp_json_type
read_string(kvp_iterator *json)
{
    if (init_string(json) != 0)
        return JSON_ERROR;
    while (1) {
        int c = json->source.get(&json->source);
        if (c == EOF) {
            json_error(json, "%s", "unterminated string literal");
            return JSON_ERROR;
        } else if (c == '"') {
            if (pushchar(json, '\0') == 0)
                return JSON_STRING;
            else
                return JSON_ERROR;
        } else if (c == '\\') {
            if (read_escaped(json) != 0)
                return JSON_ERROR;
        } else if ((unsigned) c >= 0x80) {
            if (read_utf8(json, c) != 0)
                return JSON_ERROR;
        } else {
            if (char_needs_escaping(c)) {
                json_error(json, "%s", "unescaped control character in string");
                return JSON_ERROR;
            }

            if (pushchar(json, c) != 0)
                return JSON_ERROR;
        }
    }
    return JSON_ERROR;
}



static int
read_digits(kvp_iterator *json)
{
    int c;
    unsigned nread = 0;
    while (isdigit(c = json->source.peek(&json->source))) {
        if (pushchar(json, json->source.get(&json->source)) != 0)
            return -1;

        nread++;
    }

    if (nread == 0) {
        json_error(json, "expected digit instead of byte '%c'", c);
        return -1;
    }

    return 0;
}

static enum kvp_json_type
read_number(kvp_iterator *json, int c)
{
    if (pushchar(json, c) != 0)
        return JSON_ERROR;
    if (c == '-') {
        c = json->source.get(&json->source);
        if (isdigit(c)) {
            return read_number(json, c);
        } else {
            json_error(json, "unexpected byte is '%c' in number", c);
            return JSON_ERROR;
        }
    } else if (strchr("123456789", c) != NULL) {
        c = json->source.peek(&json->source);
        if (isdigit(c)) {
            if (read_digits(json) != 0)
                return JSON_ERROR;
        }
    }
    /* Up to decimal or exponent has been read. */
    c = json->source.peek(&json->source);
    if (strchr(".eE", c) == NULL) {
        if (pushchar(json, '\0') != 0)
            return JSON_ERROR;
        else
            return JSON_NUMBER;
    }
    if (c == '.') {
        json->source.get(&json->source); // consume .
        if (pushchar(json, c) != 0)
            return JSON_ERROR;
        if (read_digits(json) != 0)
            return JSON_ERROR;
    }
    /* Check for exponent. */
    c = json->source.peek(&json->source);
    if (c == 'e' || c == 'E') {
        json->source.get(&json->source); // consume e/E
        if (pushchar(json, c) != 0)
            return JSON_ERROR;
            
        c = json->source.peek(&json->source);
        if (c == '+' || c == '-') {
            json->source.get(&json->source); // consume
            if (pushchar(json, c) != 0)
                return JSON_ERROR;
            if (read_digits(json) != 0)
                return JSON_ERROR;
        } else if (isdigit(c)) {
            if (read_digits(json) != 0)
                return JSON_ERROR;
        } else {
            json_error(json, "unexpected byte '%c' in number", c);
            return JSON_ERROR;
        }
    }
    if (pushchar(json, '\0') != 0)
        return JSON_ERROR;
    else
        return JSON_NUMBER;
}



/* Returns the next non-whitespace character in the stream. */
static int next_char(kvp_iterator *json)
{
   
   int c;
   while (isspace(c = json->source.get(&json->source)))
       if (c == '\n'){
           json->lineno++;
            printf("line: %zu", json->lineno);
          }
  // printf("\tNext pair %c:\n", c);        
   return c;
}

static enum kvp_json_type
read_value(kvp_iterator *json, int c)
{
    json->ntokens++;
    switch (c) {
    case EOF:
        json_error(json, "%s", "unexpected end of text");
        return JSON_ERROR;
    case '{':
        
        return 1;
        //return push(json, JSON_ARRAY);
    case '[':
	json_error(json, "%s", "unexpected end of text");
        return JSON_ERROR;
    case '"':
         json->type = JSON_STRING;
        return read_string(json);
    case 'n':
         json->type = JSON_NULL;
        if (pushchar(json, '0') != 0)
           return JSON_ERROR;
           
        return is_match_pair(json, "ull", JSON_NULL);
    case 'f': 
         json->type = JSON_FALSE;
         if (pushchar(json, '0') != 0)
           return JSON_ERROR;
	    return is_match_pair(json, "alse", JSON_FALSE);
    case 'F':
        if (pushchar(json, '0') != 0)
           return JSON_ERROR;
         json->type = JSON_FALSE;
        return is_match_pair(json, "ALSE", JSON_FALSE);
    case 't': 
         json->type = JSON_TRUE;
         if (pushchar(json, '1') != 0)
           return KV_ERROR;
        return is_match_pair(json, "rue", JSON_TRUE);
    case 'T':
         json->type = JSON_TRUE;
         if (pushchar(json, '1') != 0)
           return JSON_ERROR;
        return is_match_pair(json, "RUE", JSON_TRUE);
    case '0':  //pass through
    case '1':  //pass through
    case '2':  //pass through
    case '3':  //pass through
    case '4':  //...
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-': //pass through
        if (init_string(json) != 0)
            return JSON_ERROR;
        json->type = JSON_NUMBER;
    
        return read_number(json, c);
        
    default:
        json_error(json, "unexpected byte '%c' in value", c);
        return JSON_ERROR;
    }
}

void print_kvp(kvp_iterator *json){
        printf(" data: %s(%zu), %d ", json->data.string, json->data.string_size, json->type);
}

enum kvp_json_type kvp_next(kvp_iterator *json)
{

    if (json->flags & JSON_FLAG_ERROR)
        return JSON_ERROR;
       
    int c = next_char(json);
    
    if(c==EOF) return JSON_END;
    
    int c_next;
    
    //printf(" js %d",json->type);
    
    switch(c){
    
    	case '{':
    	   json->figure_brackets++;
    	   if(json->figure_brackets!=1) {
    	     json_error(json, "%s", "invalid parser state");
  	         return JSON_ERROR;
    	   }
    	   json->isKey = true;  
    	   c_next = next_char(json);
           //sprintf(" n=%c",c_next);

    	   enum kvp_json_type r = read_value(json,c_next);
           //printf("%d", r );
           //print_kvp(json);
           return r;
           
    	case ',':
    	   json->comas++;

    	   if(json->figure_brackets!=1 || json->comas != json->semis){
    	     json_error(json, "%s", "invalid parser state");
  	          return JSON_ERROR;
    	   }
    	   json->isKey = true;  
    	   c_next = next_char(json);
           
    	   return read_value(json,c_next);
        case ':':
           json->semis++;
           json->ntokens++;
    	   json->isKey = false;  
    	   c_next = next_char(json);
           
    	   return read_value(json,c_next);
    	case '}':
    	   json->figure_brackets--;
    	   if(json->figure_brackets!=0) {
    	     json_error(json, "%s", "invalid parser state");
  	         return JSON_ERROR;
    	   }
           json->comas = 0;
           json->semis = 0;
           //c_next = next_pair(json);
           
    	   return kvp_next(json);  
     
        default:
             json_error(json, "%s", "invalid parser state");
  	     return JSON_ERROR;
    }
    

    json_error(json, "%s", "invalid parser state");
    return JSON_ERROR;
}

void kvp_reset_iterator(kvp_iterator *json)
{
    //json->lineno = 0;
    json->ntokens = 0;
    json->comas = 0;
    json->semis = 0;
    json->figure_brackets = 0;
    json->type = 0;
    //json->flags &= ~JSON_FLAG_ERROR;
    json->errmsg[0] = '\0';
    
}

/*
enum json_type json_skip(json_stream *json)
{
    enum json_type type = json_next(json);
    size_t cnt_arr = 0;
    size_t cnt_obj = 0;

    for (enum json_type skip = type; ; skip = json_next(json)) {
        if (skip == JSON_ERROR || skip == JSON_DONE)
            return skip;

        if (skip == JSON_ARRAY) {
            ++cnt_arr;
        } else if (skip == JSON_ARRAY_END && cnt_arr > 0) {
            --cnt_arr;
        } else if (skip == JSON_OBJECT) {
            ++cnt_obj;
        } else if (skip == JSON_OBJECT_END && cnt_obj > 0) {
            --cnt_obj;
        }

        if (!cnt_arr && !cnt_obj)
            break;
    }

    return type;
}

enum json_type json_skip_until(json_stream *json, enum json_type type)
{
    while (1) {
        enum json_type skip = json_skip(json);

        if (skip == JSON_ERROR || skip == JSON_DONE)
            return skip;

        if (skip == type)
            break;
    }

    return type;
}
*/

const char *kvp_get_string(kvp_iterator *json, size_t *length)
{
    if (length != NULL)
        *length = json->data.string_fill;
    if (json->data.string == NULL)
        return "";
    else
        return json->data.string;
}

size_t kvp_save_string(kvp_iterator *json, char *buf)
{
    
    if (json->data.string == NULL){
        strcpy(buf, "");
        return 0;
    }
    else{
        strcpy(buf, json->data.string);
        return json->data.string_size;
    }
}


double kvp_get_number(kvp_iterator *json)
{
    char *p = json->data.string;
    return p == NULL ? 0 : strtod(p, NULL);
}

int kvp_get_int(kvp_iterator *json){
    char *p = json->data.string;
    return p == NULL ? 0 : strtol(p, (char **)NULL, 10);
}


bool kvp_get_value(kvp_iterator *json, char* buf, size_t *length)
{
    
   switch(json->type){
      case JSON_STRING:
           memcpy(buf, kvp_get_string(json, length), *length); 
           return true;
      case JSON_NUMBER:
           *length = sizeof(double); 
           double d = kvp_get_number(json);
           snprintf(buf,*length, "%g",d);
           return true;
      case JSON_FALSE:
            *length = 1;
            buf[0] = '0';
            buf[1] = '\0';
            return true;
      case JSON_TRUE:
           *length = 1;
            buf[0] = '1';
            buf[1] = '\0';
            return true;  
      case JSON_NULL:
            *length = 1;
            buf[0] = '0';
            buf[1] = '\0';
            return true;
            
   }
   return false;
}

enum kvp_json_type kvp_get_type(kvp_iterator *json){
    return json->type;
}


const char *kvp_get_error(kvp_iterator *json)
{
    return json->errmsg;
}

size_t kvp_get_lineno(kvp_iterator *json)
{
    return json->lineno;
}

/*
static int user_get_pair(struct kvp_iterator *json)
{
    return json->source.user.get(json->source.user.ptr);
}

static int user_peek_pair(struct kvp_iterator *json)
{
    return json->source.user.peek(json->source.user.ptr);
}

void json_open_user_pair(kvp_iterator *json, json_user_io get, json_user_io peek, void *user)
{
    init_pair(json);
    json->source.get = user_get;
    json->source.peek = user_peek;
    json->source.source.user.ptr = user;
    json->source.source.user.get = get;
    json->source.source.user.peek = peek;
}
*/
void kvp_set_allocator(kvp_iterator *json, kvp_allocator *a)
{
    json->alloc = *a;
}

void kvp_set_streaming(kvp_iterator *json, bool streaming)
{
    if (streaming)
        json->flags |= JSON_FLAG_STREAMING;
    else
        json->flags &= ~JSON_FLAG_STREAMING;
}

void kvp_close(kvp_iterator *json)
{
    //json->alloc.free(json->stack);
    json->alloc.free(json->data.string);
}



