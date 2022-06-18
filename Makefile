.PHONY: clean All

All: clean test_tlv test_hash test_stream test_json kvp2tlv
	

test_tlv: 
	gcc test_tlv.c tlv_work.c key_list.c -o test_tlv

test_hash:
	gcc test_hash.c kvphash_table.c -o test_hash


test_stream:
	gcc stream_test.c kvp_parser.c -o test_stream

test_json:
	gcc tests_json.c kvp_parser.c -o test_json
	
kvp2tlv:  
	gcc tlv_work.c key_list.c kvphash_table.c kvp_parser.c kvp2tlv.c -o kvp2tlv

clean:
	rm -rf *.o test_json test_stream test_hash test_tlv


