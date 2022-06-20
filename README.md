# Parsers\

Small library to parse JSON KeyValue pairs into TLV file

## Compile:
```
make
```

## Run:
```
kvp2tlv data.tlv test.json keys.json
```

-  First file (obligatory) - output tlv file
-  Second file (optional) - input file of Key Value pairs  (if the file is not set - You can input from console)
-  Third file (optional) - input file of Key Value for encoded key values (if the file is not set the keys numerated 1,2,3,etc.)






