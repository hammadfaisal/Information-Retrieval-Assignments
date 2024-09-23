#!/bin/bash

coll_path=$1
tok_type=$2

doc_file="cord19-trec_covid-docs"
dict_file="output.dict"

coll_path="${coll_path}/${doc_file}"
clean_path="clean_docs"

# branch on tokenizer type
./remove_redundant $coll_path $clean_path

if [ $tok_type == "0" ]; then
    # simple tokenizer
    ./simple_tokenizer $clean_path $dict_file
elif [ $tok_type == "1" ]; then
    # bpe tokenizer
    ./bpe_tokenizer $clean_path 300 $dict_file
elif [ $tok_type == "2" ]; then
    # word piece tokenizer
    ./word_piece_tokenizer $clean_path 500 $dict_file
else
    echo "Invalid tokenizer type"
fi
