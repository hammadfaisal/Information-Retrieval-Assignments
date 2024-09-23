#!/bin/bash

g++ -O3 bpe_tokenizer.cpp -o bpe_tokenizer
g++ -O3 simple_tokenizer.cpp -o simple_tokenizer
g++ -O3 remove_redundant.cpp -o remove_redundant
g++ -O3 word_piece_tokenizer.cpp -o word_piece_tokenizer
g++ -O3 construct_inverted_index.cpp -o construct_inverted_index
g++ -O3 tf_idf_search.cpp -o tf_idf_search
