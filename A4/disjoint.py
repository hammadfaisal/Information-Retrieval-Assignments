import os
import openai
from dotenv import load_dotenv
import re
import random
import sys

# Usage python disjoint.py <passage_file> <test_query_file> <train_query_file> <top_docs_file> <output_file>

# env_file = "keys.env"

# load_dotenv(env_file, override=True)


client = openai.OpenAI(
    base_url="https://api.groq.com/openai/v1",
    api_key=os.environ.get("GROQ_KEY_1")
)
# model = "llama"

# passage_file = "diverse_examples.tsv"
passage_file = sys.argv[1]

examples = []
with open(passage_file, "r") as f:
    for line in f:
        examples.append(line.strip().split("\t"))

qfile = sys.argv[2]
example_qfile = sys.argv[3]

query2id = {}
with open(example_qfile, "r") as f:
    for line in f:
        qid, query = line.strip().split("\t")
        query2id[query] = qid
test_queries = []
with open(qfile, "r") as f:
    for line in f:
        test_queries.append(line.strip().split("\t"))

def get_passage(model, prompt, system_prompt):
    completion = client.chat.completions.create(
        model=model,
        messages=[
            {
                "role": "system",
                "content": system_prompt
            },
            {
                "role": "user",
                "content": prompt
            }
        ],
        temperature=0.5,
        max_tokens=128,
        top_p=1,
        stream=True,
        stop=None,

    )
    text = ""
    for chunk in completion:
        text += chunk.choices[0].delta.content or ""
    # replace newline characters with space
    # text.replace("\n", " ")
    # text.replace("\t", " ")
    text = re.sub(r'\n+', ' ', text)
    text = re.sub(r'\t+', ' ', text)
    # remove extra spaces
    text = re.sub(' +', ' ', text)
    # print(text)
    # print("\n\n\n\n\n")
    return text

model = "llama-3.1-70b-versatile"
outfile = sys.argv[5]
with open(outfile, "w") as f:
    f.write("")

def load_top_docs(top_docs_file):
    top_docs = {}
    with open(top_docs_file, 'r', encoding='utf-8') as f:
        for line in f:
            qid, doc_id, _ = line.strip().split('\t')
            if qid not in top_docs:
                top_docs[qid] = []
            top_docs[qid].append(doc_id)
    return top_docs
top_docs_file = sys.argv[4]
top_docs = load_top_docs(top_docs_file)



for q in test_queries:
    prompt = ""
    # format "Query: <query>\nPassage: <passage>\n\n"
    system_prompt = "Write a passage that answers only the last query. Do not give answers for example queries."
    for k in range(0, 5):
        # choose random k-shot examples
        # cur_example = random.sample(examples, k)
        disj_set = []
        for ex in examples:
            # no terms common
            if len(set(ex[0].split()).intersection(set(q[1].split()))) == 0:
                # no common top100 docs
                if len(set([x for x in top_docs[q[0]]]).intersection(set([x for x in top_docs[query2id[ex[0]]]]))) == 0:
                    disj_set.append(ex)
        cur_example = random.sample(disj_set, k)
        prompt = ""
        for ex in cur_example:
            prompt += f"Example Query: {ex[0]}\nExample Passage: {ex[1]}\n\n"
        prompt += f"Query: {q[1]}\nPassage: "
        # print(prompt)
        passage = get_passage(model, prompt, system_prompt)
        # replace newline with \n
        # passage = passage.replace("\n", "\\n")
        prompt = prompt.replace("\n", "\\n")
        with open(outfile, "a") as f:
            f.write(f"{q[0]}\t{q[1]}\t{prompt}\t{passage}\n")

    



