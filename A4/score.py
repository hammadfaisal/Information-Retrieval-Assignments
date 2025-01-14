from rank_bm25 import BM25Okapi
from tqdm import tqdm
import re
from sklearn.metrics import ndcg_score
import sys
import nltk
from nltk.corpus import stopwords
from nltk.stem import WordNetLemmatizer
from nltk.tokenize import word_tokenize

nltk.download('punkt')

# Usage : python3 score.py <doc_file> <llm_out_file> <top_docs_file> <qrel_file> <result_file>

def preprocess(doc_text):
    # Lowercasing
    doc_text = doc_text.lower()
    
    # Remove extra whitespace
    doc_text = re.sub(r'\s+', ' ', doc_text).strip()
    
    # Tokenization
    tokens = word_tokenize(doc_text)
    
    # Remove stopwords
    stop_words = set(stopwords.words('english'))
    tokens = [word for word in tokens if word not in stop_words]
    
    # Lemmatization
    lemmatizer = WordNetLemmatizer()
    tokens = [lemmatizer.lemmatize(word) for word in tokens]
    
    # Remove non-alphanumeric tokens
    tokens = [word for word in tokens if word.isalpha()]
    
    return tokens

# Load documents
def load_documents(doc_file):
    doc_dict = {}
    with open(doc_file, 'r', encoding='utf-8') as f:
        for line in tqdm(f, desc="Loading documents"):
            doc_id, doc_text = line.strip().split('\t', 1)
            doc_dict[doc_id] = doc_text
    return doc_dict

# Load queries
def load_queries(query_file, k):
    queries = {}
    start = k
    skip = 5
    
    with open(query_file, 'r', encoding='utf-8') as f:
        l = f.readlines()
        for line in l[start::skip]:
            qid, query,_, result = line.strip().split('\t')
            result = ' '.join(preprocess(result))

            queries[qid] = (query+' ')*5 + result
            # print(qid, queries[qid])
            
    return queries

# Load top 100 docs
def load_top_docs(top_docs_file):
    top_docs = {}
    with open(top_docs_file, 'r', encoding='utf-8') as f:
        for line in f:
            qid, doc_id, _ = line.strip().split('\t')
            if qid not in top_docs:
                top_docs[qid] = []
            top_docs[qid].append((doc_id, 0))
    return top_docs

def add_relevance(top_docs, qrel_file):
    qrel = {}
    with open(qrel_file, 'r', encoding='utf-8') as f:
        for line in f:
            qid, doc_id, rel, _ = line.strip().split('\t')
            qrel[qid+doc_id] = int(rel)
            # print(qid+doc_id, int(rel))
    for qid in top_docs:
        for i in range(len(top_docs[qid])):
            doc_id = top_docs[qid][i][0]
            if qid+doc_id in qrel:
                top_docs[qid][i] = (doc_id, qrel[qid+doc_id])
            # else:
            #     print("Not found", qid, doc_id)
ndcg_score_at = [5,10,50]
ndcg_score_avg = [0,0,0]

# Perform BM25-based reranking
def bm25_rerank(queries, documents, top_docs, output_file, k1=4.46, b=0.82):
    for qid, query in tqdm(queries.items(), desc="Reranking queries"):
        doc_ids= top_docs.get(qid, [])
        # list of tuples (doc_id, score)
        # sort by score
        doc_ids.sort(key=lambda x: x[1], reverse=True)
        # print(qid, len(doc_ids))
        
        corpus = [documents[doc_id[0]].split() for doc_id in doc_ids if doc_id[0] in documents]
        # print(len(corpus))
        bm25 = BM25Okapi(corpus, k1=k1, b=b)
        query_terms = query.split()
        scores = bm25.get_scores(query_terms)
        
        # Combine document IDs with their scores
        doc_scores = list(zip(doc_ids, scores))
        # Sort by score in descending order

        score_list_pred = [x[1] for x in doc_scores]
        score_list = [x[1] for x in doc_ids]
        # print(score_list[:10])
        for i in range(3):
            ndcg_score_avg[i] += ndcg_score([score_list], [score_list_pred],k=ndcg_score_at[i])

        # doc_scores.sort(key=lambda x: x[1], reverse=True)

        # # Write results to output file
        # for rank, (doc_id, score) in enumerate(doc_scores, start=1):
        #     out.write(f"{qid}\t{doc_id}\t{rank}\t{score:.4f}\n")

    for i in range(3):
        ndcg_score_avg[i] /= len(queries)
    with open(output_file, 'a', encoding='utf-8') as out:
        out.write(f"Average NDCG@5: {ndcg_score_avg[0]}\n")
        out.write(f"Average NDCG@10: {ndcg_score_avg[1]}\n")
        out.write(f"Average NDCG@50: {ndcg_score_avg[2]}\n\n\n")

# File paths
# doc_file = 'preprocessed_useful_docs.tsv'
# query_file = 'diverse_disj.tsv'
# top_docs_file = '/home/cse/btech/cs1210559/scratch/COL764A4/data/top100docs.tsv'
# output_file = 'diverse_disj_results.tsv'
# qrel_file = '/home/cse/btech/cs1210559/scratch/COL764A4/data/qrels.tsv'
doc_file = sys.argv[1]
query_file = sys.argv[2]
top_docs_file = sys.argv[3]
output_file = sys.argv[5]
qrel_file = sys.argv[4]

for k in range(0,5):
# Load data
    documents = load_documents(doc_file)
    queries = load_queries(query_file, k)
    top_docs = load_top_docs(top_docs_file)
    add_relevance(top_docs, qrel_file)

    # Perform reranking
    with open(output_file, 'a', encoding='utf-8') as out:
        out.write(f"Reranking with k={k}\n")
    bm25_rerank(queries, documents, top_docs, output_file)

    # print(f"Reranked results written to {output_file}")
