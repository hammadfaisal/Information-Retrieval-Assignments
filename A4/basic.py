import os
import openai
from dotenv import load_dotenv
import re
import random
import sys

#Usage python basic.py <passage_file> <test_query_file> <output_file>

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
# outfile = "div_rand.tsv"
outfile = sys.argv[3]
with open(outfile, "w") as f:
    f.write("")

for q in test_queries:
    prompt = ""
    # format "Query: <query>\nPassage: <passage>\n\n"
    system_prompt = "Write a passage that answers only the last query. Do not give answers for example queries."
    for k in range(0, 5):
        # choose random k-shot examples
        cur_example = random.sample(examples, k)
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

    



