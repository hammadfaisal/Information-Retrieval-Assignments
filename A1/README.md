# build.sh
compiles all the files

- to run, call dictcons.sh, invidx.sh, tf_idf_search.sh in the format given in assignment.

eg.
```
bash build.sh
bash dictcons.sh 1
bash invidx.sh train_data indexfile 1
bash tf_idf_search.sh train_data/cord19-trec_covid-queries outputfile indexfile.idx indexfile.dict
```