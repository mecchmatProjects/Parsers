#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kvp_parser.h"

#if _WIN32
#  define C_RED(s)   s
#  define C_GREEN(s) s
#  define C_BOLD(s)  s
#else
#  define C_RED(s)   "\033[31;1m" s "\033[0m"
#  define C_GREEN(s) "\033[32;1m" s "\033[0m"
#  define C_BOLD(s)  "\033[1m"    s "\033[0m"
#endif

struct expect {
    enum kvp_json_type type;
    const char *str;
};

#define countof(a) (sizeof(a) / sizeof(*a))

#define TEST_IMPL(name, stream, sep) \
    do { \
        int r = test(name, stream, sep, seq, countof(seq), str, sizeof(str) - 1); \
        if (r) \
            count_pass++; \
        else \
            count_fail++; \
    } while (0)

#define TEST(name)             TEST_IMPL(name, false, '\0')
#define TEST_STREAM(name, sep) TEST_IMPL(name, true, sep)

const char json_typename[][16] = {
    [JSON_ERROR]      = "ERROR",
    [JSON_END]       = "END",
   // [JSON_OBJECT]     = "OBJECT",
  //  [JSON_OBJECT_END] = "OBJECT_END",
  //  [JSON_ARRAY]      = "ARRAY",
  //  [JSON_ARRAY_END]  = "ARRAY_END",
    [JSON_STRING]     = "STRING",
    [JSON_NUMBER]     = "NUMBER",
    [JSON_TRUE]       = "TRUE",
    [JSON_FALSE]      = "FALSE",
    [JSON_NULL]       = "NULL",
};

static int
has_value(enum kvp_json_type type)
{
    return type == JSON_STRING || type == JSON_NUMBER;
}

int kvp_source_get(kvp_iterator *json)
{
    int c = json->source.get(&json->source);
    if (c == '\n')
        json->lineno++;
    return c;
}

int kvp_source_peek(kvp_iterator *json)
{
    return json->source.peek(&json->source);
}

void print_kvp2(kvp_iterator *json){
        printf(" data: %s(%zu), %d ", json->data.string, json->data.string_size, json->type);
}

static int
test(const char *name,
     int stream,
     char sep,
     struct expect *seq,
     size_t seqlen,
     const char *buf,
     size_t len)
{
    int success = 1;
    struct kvp_iterator json2;
    enum kvp_json_type expect, actual;
    const char *expect_str, *actual_str;

   // kvp_open_string(&json, buf);
    kvp_open_string(&json2, buf);
    kvp_set_streaming(&json2, stream);
   
    printf("start  %s \n", buf);
    for (size_t i = 0; success && i < seqlen; i++) {
        expect = seq[i].type;
        printf("%d", expect);
        print_kvp2(&json2);
        //actual = kvp_next(json);
        actual = kvp_next(&json2);
        //actual = kvp_next(&json);
        print_kvp2(&json2);
        
        actual_str = has_value(actual) ? kvp_get_string(&json2, 0) : "";
        printf("%s", actual_str);
        expect_str = seq[i].str ? seq[i].str : "";

        if (actual != expect)
            success = 0;
        else if (seq[i].str && !!strcmp(expect_str, actual_str))
            success = 0;
        else if (stream && actual == JSON_END) {
            if (sep != '\0') {
                int c = '\0';
                while (isspace(c = kvp_source_peek(&json2))) {
                    kvp_source_get(&json2);
                    if (c == sep)
                        break;
                }
                if (c != sep && c != EOF)
                    success = 0;
            }
            kvp_reset_iterator(&json2);
        }
    }

    if (success) {
        printf(C_GREEN("PASS") " %s\n", name);
    } else {
        printf(C_RED("FAIL") " %s: "
               "expect " C_BOLD("%s") " %s / "
               "actual " C_BOLD("%s") " %s\n",
               name,
               json_typename[expect], expect_str,
               json_typename[actual], actual_str);
    }
    kvp_close(&json2);
    return success;
}

int main()
{
    int count_pass = 0;
    int count_fail = 0;

    //{
        const char str[] = "{ \"x\":123 } ";
        struct expect seq[] = {
            {JSON_STRING, "x"},
            {JSON_NUMBER, "123"},
            {JSON_END},
        };
     
      struct kvp_iterator json;
    kvp_open_string(&json, str);
    kvp_set_streaming(&json, true);
    
   enum kvp_json_type expect, actual;
    const char *expect_str, *actual_str;
    int seqlen = 3;
        int success = 1;
        int stream =1;
    char sep='\0';

    printf("start  %s \n", str);
   
    for (size_t i = 0; success && i < seqlen; i++) {
        expect = seq[i].type;
        printf("%d", expect);
        print_kvp2(&json);
        //actual = kvp_next(json);
        actual = kvp_next(&json);
        //actual = kvp_next(&json);
        print_kvp2(&json);
        
        actual_str = has_value(actual) ? kvp_get_string(&json, 0) : "";
        printf("%s", actual_str);
        expect_str = seq[i].str ? seq[i].str : "";

        if (actual != expect)
            success = 0;
        else if (seq[i].str && !!strcmp(expect_str, actual_str))
            success = 0;
        else if (stream && actual == JSON_END) {
            if (sep != '\0') {
                int c = '\0';
                while (isspace(c = kvp_source_peek(&json))) {
                    kvp_source_get(&json);
                    if (c == sep)
                        break;
                }
                if (c != sep && c != EOF)
                    success = 0;
            }
            kvp_reset_iterator(&json);
        }
    }

    if (success) {
        printf(C_GREEN("PASS") " %s\n", "first test");
    } else {
        printf(C_RED("FAIL") " %s: "
               "expect " C_BOLD("%s") " %s / "
               "actual " C_BOLD("%s") " %s\n",
               "first test",
               json_typename[expect], expect_str,
               json_typename[actual], actual_str);
    }
    kvp_close(&json);
    


}
