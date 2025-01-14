import random
import pandas as pd
import sys

# Usage: python rand_passage.py <docs_file> <train_query_file> <qrels_file> <out_file>

# Step 1: Load the data
def load_docs(file_path):
    docs = {}
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            try :
                doc_id, _,_,text = line.strip().split('\t')
                docs[doc_id] = text
            except:
                print("Empty line", line.strip().split('\t')[0])
    return docs

def load_qrels(file_path, qids):
    qrels = {}
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            query_id, doc_id, rel, _ = line.strip().split()
            if query_id not in qids:
                continue
            if query_id not in qrels:
                qrels[query_id] = []
            if (rel != '0'):
                qrels[query_id].append(doc_id)
    return qrels

# Step 2: Random passage extraction
def extract_passages(docs, qrels, num_passages=5):
    results = {}
    for query_id, relevant_docs in qrels.items():
        results[query_id] = []
        for _ in range(num_passages):
            # Randomly select a relevant document
            doc_id = random.choice(relevant_docs)
            content = docs.get(doc_id, "")
            # Split document into sentences
            sentences = content.split('. ')
            start_idx = random.randint(0, len(sentences)-1)
            # Select a passage with up to 4 sentences
            passage = '. '.join(sentences[start_idx:start_idx+4])
            results[query_id].append(passage)
    return results

# Step 3: Execution
docs_file = sys.argv[1]
query_file1 = sys.argv[2]
qrels_file = sys.argv[3]

qids = set()
qdi2query = {}
with open(query_file1, 'r') as f:
    for line in f:
        qid, query = line.strip().split("\t")
        qids.add(qid)
        qdi2query[qid] = query
docs = load_docs(docs_file)
qrels = load_qrels(qrels_file, qids)
passages = extract_passages(docs, qrels)

out_file = sys.argv[4]
with open(out_file, 'w') as f:
    for query_id, passage_list in passages.items():
        # print(len(passage_list))
        for i, passage in enumerate(passage_list):
            f.write(f"{qdi2query[query_id]}\t{passage}\n")

