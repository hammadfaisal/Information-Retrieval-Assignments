#!/bin/bash

query_file=$1
result_file=$2
index_file=$3
dict_file=$4

./tf_idf_search $query_file $result_file $index_file $dict_file