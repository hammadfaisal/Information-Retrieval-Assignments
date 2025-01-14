import pandas as pd
from tqdm import tqdm
import re
import nltk
from nltk.corpus import stopwords
from nltk.stem import WordNetLemmatizer
from nltk.tokenize import word_tokenize
import sys

nltk.download('punkt')

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
            doc_dict[doc_id] = ' '.join(preprocess(doc_text))
    return doc_dict

# File paths
doc_file1 = sys.argv[1]
preprocessed_doc_file1 = sys.argv[2]

# Load and preprocess documents
doc_dict1 = load_documents(doc_file1)

# Save preprocessed documents
with open(preprocessed_doc_file1, 'w', encoding='utf-8') as f:
    for doc_id, doc_text in tqdm(doc_dict1.items(), desc="Saving preprocessed documents"):
        f.write(f'{doc_id}\t{doc_text}\n')
    
