#!/bin/bash

coll_path=$1
indexfile=$2
tok_type=$3

doc_file="cord19-trec_covid-docs"

coll_path="${coll_path}/${doc_file}"

dict_file="${indexfile}.dict"

clean_path="clean_docs"

./remove_redundant $coll_path $clean_path

# branch on tokenizer type

# contruct dictonary and encode documents
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

./construct_inverted_index $clean_path $indexfile $tok_type