#include <iostream>
#include <algorithm>
#include <cctype>
#include <queue>
#include <sstream>
#include <cmath>
#include <utility>
#include "tokenize.cpp"

std::vector<std::vector<std::pair<std::string, int>>> readPostingsList(const std::string& filename) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        throw std::runtime_error("Unable to open file for reading.");
    }

    std::vector<std::vector<std::pair<std::string, int>>> postings_list;

    // Read the size of the outer vector
    size_t outer_size;
    infile.read(reinterpret_cast<char*>(&outer_size), sizeof(outer_size));
    postings_list.resize(outer_size);

    for (auto& inner_vec : postings_list) {
        // Read the size of the inner vector
        size_t inner_size;
        infile.read(reinterpret_cast<char*>(&inner_size), sizeof(inner_size));
        inner_vec.resize(inner_size);

        for (auto& pair : inner_vec) {
            // Read the size of the string
            size_t str_size;
            infile.read(reinterpret_cast<char*>(&str_size), sizeof(str_size));
            // Read the string
            std::string str(str_size, '\0');
            infile.read(&str[0], str_size);
            // Read the integer
            int value;
            infile.read(reinterpret_cast<char*>(&value), sizeof(value));
            pair = std::make_pair(std::move(str), value);
        }
    }

    infile.close();
    return postings_list;
}

std::vector<std::string> extract_val_from_json(std::set<std::string> &req_keys , const std::string &json_str) {
    std::vector<std::string> res;
    std::string key;
    std::string val;
    bool key_found = false;
    bool next_key = true;
    bool val_found = false;
    bool escaped = false;
    for (int i = 0; i < json_str.size(); i++) {
        if (!escaped && json_str[i] == '\"') {
            if (next_key) {
                if (key_found) {
                    key_found = false;
                    next_key = false;
                }
                else {
                    key.clear();
                    key_found = true;
                }
            } else {
                if (val_found) {
                    val_found = false;
                    next_key = true;
                    if (req_keys.find(key) != req_keys.end()) {
                        // lowercase the value
                        for (int j = 0; j < val.size(); j++) {
                            val[j] = std::tolower(val[j]);
                        }
                        res.push_back(val);
                    }
            
                }
                else {
                    val.clear();
                    val_found = true;
                }
            }
        }
        else {
            if (!escaped && json_str[i] == '\\') {
                escaped = true;
            }
            else {
                escaped = false;
            }
            
            if (key_found) {
                key.push_back(json_str[i]);
            }
            if (val_found) {
                val.push_back(json_str[i]);
            }
        }
    }
    return res;
}

int main(int argc, char* argv[]) {
    
    std::string queryfile_path = argv[1];
    std::string outputfile_path = argv[2];
    std::string indexfile_path = argv[3];
    std::string dict_file_name = argv[4];

    std::ifstream tokenizer_type_file(dict_file_name);
    std::string tokenizer_type;
    std::getline(tokenizer_type_file, tokenizer_type);
    tokenizer_type_file.close();
    int tok_type = std::stoi(tokenizer_type);

    std::vector<std::vector<std::pair<std::string, int>>> postings_list = readPostingsList(indexfile_path);

    // set of punctuation
    std::vector<bool> is_punct(128, false);
    is_punct[' '] = true;
    is_punct[','] = true;
    is_punct['.'] = true;
    is_punct[':'] = true;
    is_punct[';'] = true;
    is_punct['\"'] = true;
    is_punct['\''] = true;

    std::vector<std::string> vocab;
    trie vocab_trie;
    std::map<std::string, int> vocab_map;
    std::map<long long, int> merges;
    if (tok_type == 0) {
        readSimpleDict(dict_file_name, vocab, vocab_map);
    }
    else if (tok_type == 1) {
        readBpeDict(dict_file_name, vocab, merges, vocab_map);
    }
    else if (tok_type == 2) {
        readWordPieceDict(dict_file_name, vocab, vocab_trie);
    }
    else {
        std::cerr << "Invalid tokenizer type\n";
        exit(1);
    }

    std::set<std::string> req_keys;
    req_keys.insert("query_id");
    req_keys.insert("title");
    req_keys.insert("description");
    req_keys.insert("narrative");

    std::map<std::string, int> doc_to_id;
    std::vector<std::string> doc_id_to_name;
    std::vector<double> doc_norm;

    std::vector<std::unordered_map<int, double>> tf(postings_list.size());
    std::vector<double> idf(postings_list.size(), 0.0);

    for (int i = 0; i < postings_list.size(); i++) {
        for (int j = 0; j < postings_list[i].size(); j++) {
            if (doc_to_id.find(postings_list[i][j].first) == doc_to_id.end()) {
                doc_to_id[postings_list[i][j].first] = doc_to_id.size();
                doc_id_to_name.push_back(postings_list[i][j].first);
                doc_norm.push_back(0.0);
            }
            double tfij = 1.0 + log2(postings_list[i][j].second);
            tf[i][doc_to_id[postings_list[i][j].first]] = tfij;
            doc_norm[doc_to_id[postings_list[i][j].first]] += tfij * tfij;
        }
    }

    for (int i = 0; i < doc_norm.size(); i++) {
        doc_norm[i] = sqrt(doc_norm[i]);
    }

    for (int i = 0; i < postings_list.size(); i++) {
        idf[i] = log2(1 + (((double)doc_to_id.size()) / postings_list[i].size()));
    }

    std::ifstream queryfile(queryfile_path);
    std::ofstream outputfile(outputfile_path);

    std::string line;
    while (std::getline(queryfile, line)) {
        std::vector<std::string> res = extract_val_from_json(req_keys, line);
        std::string query_text = res[1] + " " + res[2] + " " + res[3];
        std::vector<int> tokens;
        if (tok_type == 0) {
            tokens = simpleTokenizeSentence(query_text, vocab_map, is_punct);
        }
        else if (tok_type == 1) {
            tokens = bpeTokenizeSentence(query_text, merges, vocab_map, is_punct);
        }
        else if (tok_type == 2) {
            tokens = wordPieceTokenizeSentence(query_text, vocab_trie, is_punct);
        }
        std::sort(tokens.begin(), tokens.end());
        std::vector<std::pair<double,int> > doc_scores(doc_to_id.size(),{0,0});

        int last_tok = -1;
        int last_tok_count = 0;
        for (int i = 0; i < tokens.size(); i++) {
            if (tokens[i] == last_tok) {
                last_tok_count++;
            }
            else {
                if (last_tok != -1) {
                    double weight = (1.0 + log2(last_tok_count))*idf[last_tok];
                    for (int j = 0; j < tf[last_tok].size(); j++) {
                        if (tf[last_tok].find(j) != tf[last_tok].end()) {
                            doc_scores[j].first += weight * tf[last_tok][j];
                        }
                    }

                }
                last_tok = tokens[i];
                last_tok_count = 1;
            }
        }
        if (last_tok != -1) {
            double weight = (1.0 + log2(last_tok_count))*idf[last_tok];
            for (int j = 0; j < tf[last_tok].size(); j++) {
                if (tf[last_tok].find(j) != tf[last_tok].end()) {
                    doc_scores[j].first += weight * tf[last_tok][j];
                }
            }
        }
        for (int i = 0; i < doc_scores.size(); i++) {
            doc_scores[i].first /= doc_norm[i];
            doc_scores[i].second = i;
        }
        std::sort(doc_scores.begin(), doc_scores.end(), std::greater<std::pair<double,int>>());

        for (int i = 0; i < std::min(100,(int)doc_scores.size()); i++) {
            outputfile << res[0] << "\t0\t" << doc_id_to_name[doc_scores[i].second] << "\t"<< doc_scores[i].first << "\n";
        }
    }

}