import random
import pandas as pd
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.metrics.pairwise import cosine_similarity
import numpy as np
import sys

# Usage: python diverse_passage.py <docs_file> <train_query_file> <qrels_file> <out_file>

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

# Step 2: Get the first passage
def get_first_passage(docs, doc_id):
    content = docs.get(doc_id, "")
    sentences = content.split('. ')
    return '. '.join(sentences[:3])

# Step 3: Compute diversity
def select_diverse_passages(docs, relevant_docs, num_passages=5):
    # Extract all candidate passages (3 sentences each) from relevant documents
    candidate_passages = []
    for doc_id in relevant_docs:
        content = docs.get(doc_id, "")
        sentences = content.split('. ')
        for i in range(0, len(sentences), 3):
            passage = '. '.join(sentences[i:i+3])
            if passage:  # Ignore empty passages
                candidate_passages.append(passage)

    # If there are fewer passages than needed, return them directly
    if len(candidate_passages) <= num_passages:
        return candidate_passages

    # Convert passages into term vectors using TF-IDF
    vectorizer = TfidfVectorizer()
    tfidf_matrix = vectorizer.fit_transform(candidate_passages)

    # Select the first passage (highest-ranked relevant doc, first 3 sentences)
    first_passage = candidate_passages[0]
    selected_passages = [first_passage]
    selected_indices = [0]

    # Iteratively select the most diverse passages
    for _ in range(1, num_passages):
        max_diversity = -1
        next_passage_idx = None

        for i, candidate_vector in enumerate(tfidf_matrix):
            if i in selected_indices:
                continue

            # Compute diversity as the minimum similarity to already selected passages
            similarities = cosine_similarity(
                candidate_vector,
                tfidf_matrix[selected_indices]
            ).flatten()
            diversity = -np.mean(similarities)  # Negative to maximize diversity

            if diversity > max_diversity:
                max_diversity = diversity
                next_passage_idx = i

        if next_passage_idx is not None:
            selected_passages.append(candidate_passages[next_passage_idx])
            selected_indices.append(next_passage_idx)

    return selected_passages

# Step 4: Main function
def extract_diverse_passages(docs, qrels, num_passages=5):
    results = {}
    for query_id, relevant_docs in qrels.items():
        if not relevant_docs:
            continue

        # Get diverse passages for the query
        diverse_passages = select_diverse_passages(docs, relevant_docs, num_passages)
        results[query_id] = diverse_passages
    return results

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
passages = extract_diverse_passages(docs, qrels)

out_file = sys.argv[4]

with open(out_file, 'w') as f:
    for query_id, passage_list in passages.items():
        # print(len(passage_list))
        for i, passage in enumerate(passage_list):
            f.write(f"{qdi2query[query_id]}\t{passage}\n")
