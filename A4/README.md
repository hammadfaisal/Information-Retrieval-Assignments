# Usage

All files should be in tsv format. Data order should be like MS MARCO data.

- Create passages using `diverse_passages.py` or `rand_passages.py`.
    - python diverse_passage.py <docs_file> <train_query_file> <qrels_file> <passage_file>
    - python rand_passages.py <docs_file> <train_query_file> <qrels_file> <passage_file>

- Use the passages to prompt LLM
    - python basic.py <passage_file> <test_query_file> <llm_out_file>
    - python disjoint.py <passage_file> <test_query_file> <train_query_file> <top_docs_file> <llm_out_file>

- Preprocess docs before reranking
    - python preprocess.py <docs_file> <preprocessed_docs_file>

- Rerank using BM25
    - python score.py <preprocessed_docs_file> <llm_out_file> <top_docs_file> <qrel_file> <result_file>