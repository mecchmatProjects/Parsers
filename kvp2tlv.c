#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "key_list.h"
#include "kvp_parser.h"
#include "kvphash_table.h"
#include "tlv_work.h"

int main(int argc, char* argv[])
{
    kvp_iterator json;
    kvp_iterator dict;
    bool dict_from_file = false;

    kvphash_table* dict_keys = ht_create();
    if(dict_keys == NULL) {
        return EXIT_BAD_HASH_TABLE;
    }

    if(argc < 2 || argc > 4) {

        printf("USAGE: %s output_TLV_file [KVP_input_file] [dict_json_file]\n", argv[0]);
        printf("where output_TLV_file  - tlv file for output;\n");
        printf("KVP_input_file - input file with KV pairs in JSON style\n");
        printf(" if it is not present - we must input KV in console;\n");
        printf("dict_json_file - input file with Keys values pairs in th same JSON style\n");
        printf(" if it is not present - each key will be sequentially numbered 1,2,3...;\n");
        return EXIT_WRONG_ARG_COUNT;

    } else if(argc == 2) {
        // if argc==2 we input KVP from console
        kvp_open_stream(&json, stdin);

    } else {
        // input from file
        FILE* file_json_kvp = fopen(argv[2], "rb");

        if(!file_json_kvp) {
            printf("ERROR: cannot open file %s for read\n", argv[2]);
            return EXIT_BAD_FILE_NAME;
        }
        kvp_open_stream(&json, file_json_kvp);

        // if we predefine the dictionary of keys
        if(argc == 4) {
            // input keys values form the file
            dict_from_file = true;
            FILE* file_json_dict = fopen(argv[3], "rb");

            if(!file_json_dict) {
                printf("ERROR: cannot open file %s for read\n", argv[3]);
                return EXIT_BAD_FILE_NAME;
            }
            kvp_open_stream(&dict, file_json_dict);

            // dictionary should be as single json dict
            enum kvp_json_type result = 0;
            size_t len;

            while(dict.type != JSON_ERROR) {
                result = kvp_next(&dict);
                if(result == JSON_ERROR) {
                    return EXIT_JSON_ERROR;
                }
                if(result == JSON_END) {
                    break;
                }

                void* value = ht_get(dict_keys, kvp_get_string(&dict, &len));
                if(value != NULL) {
                    printf("We alredy had this value %s", kvp_get_string(&dict, &len));
                    continue;
                }
                // allocate space for new int and set it to count
                int* val = malloc(sizeof(int));
                if(val == NULL) {
                    return EXIT_BAD_MALLOC;
                }
                char* buf = malloc(len + 1);
                size_t n = kvp_save_string(&dict, buf);

                result = kvp_next(&dict);
                if(result == JSON_ERROR) {
                    return EXIT_JSON_ERROR;
                }
                if(result == JSON_END) {
                    break;
                }

                *val = kvp_get_int(&dict);
                if(ht_set(dict_keys, buf, val) == NULL) {
                    return EXIT_BAD_MALLOC;
                }
                free(buf);
            }

            kvp_close(&dict);
        }
    }

    // else - if we do not predifine them
    // the values should be sequentially 1,2,3,... etc

    int count_keys = 0;

    kvp_set_streaming(&json, false);

    // open file for writing
    FILE* tlv_to_write = fopen(argv[1], "wb");
    if(!tlv_to_write) {
        printf("ERROR: cannot open file %s for writing\n", argv[1]);
        return EXIT_BAD_FILE_NAME;
    }

    enum kvp_json_type result = 0;

    printf("read the KV pairs and write them:\n");
    while(json.type != JSON_ERROR) {

        result = kvp_next(&json);

        if(result == JSON_ERROR) {
            return EXIT_JSON_ERROR;
        }
        if(result == JSON_END) {
            break;
        }

        size_t len = 0;
        printf("\nkey=%s ", kvp_get_string(&json, &len));

        void* value = ht_get(dict_keys, kvp_get_string(&json, &len));
        if(value == NULL) {
            // no key in hashtable, increment counter
            count_keys++;

            // allocate space for new int and set it to count
            int* pcount = malloc(sizeof(int));
            if(pcount == NULL) {
                return EXIT_BAD_MALLOC;
            }
            *pcount = count_keys;
            if(ht_set(dict_keys, kvp_get_string(&json, &len), pcount) == NULL) {
                return EXIT_BAD_MALLOC;
            }
            // free(pcount);
        }

        // output data into TLV file
        //   write_data(argv[1], ht_get(dict_keys, json->data));

        tlv_write_file(NUMBER_TLV, 1, ht_get(dict_keys, kvp_get_string(&json, &len)), tlv_to_write);

        result = kvp_next(&json);
        if(result == JSON_ERROR) {
            return EXIT_JSON_ERROR;
        }
        if(result == JSON_END) {
            break;
        }

        char* buf = malloc(json.data.string_size);
        kvp_get_value(&json, buf, &len);

        printf("value=%s", buf);

        bool x;
        int y;
        switch(json.type) {
        case JSON_STRING:
            tlv_write_file(STRING_TLV, json.data.string_size, json.data.string, tlv_to_write);
            break;

        case JSON_NUMBER: // TODO: int union
            tlv_write_file(NUMBER_TLV, 1, json.data.string, tlv_to_write);
            break;

        case JSON_TRUE:
            x = true;
            tlv_write_file(BOOL_TLV, 1, &x, tlv_to_write);
            break;
        case JSON_FALSE:
            x = false;
            tlv_write_file(BOOL_TLV, 1, &x, tlv_to_write);
            break;

        case JSON_NULL:
            y = 0;
            tlv_write_file(NUMBER_TLV, 1, &y, tlv_to_write);
            break;
        default:
            printf("Unknown type %d", (int)(json.type));
            return EXIT_JSON_ERROR;
        }
        free(buf);

        // output data into TLV file
        //   write_data(argv[1], json->data);
    }

    if(result == JSON_ERROR) {
        fprintf(stderr, "error: %zu: %s\n", kvp_get_lineno(&json), kvp_get_error(&json));
        return EXIT_JSON_ERROR;
    } else {
        printf("\n");
    }
    kvp_close(&json);

    // Print out keys dict
    hti it = ht_iterator(dict_keys);

    printf("write keys values at the end:\n");
    while(ht_next(&it)) {
        printf("\n %s , %d", it.key, (int)*((int*)it.value));

        tlv_write_file(STRING_TLV, strlen(it.key), (void*)it.key, tlv_to_write);
        tlv_write_file(NUMBER_TLV, 1, (int*)it.value, tlv_to_write);
        free(it.value);
    }

    ht_destroy(dict_keys);

    fclose(tlv_to_write);

    return EXIT_NO_ERRORS;
}
