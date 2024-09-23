// Simple tokenizer: Use <white-space> and ,.:;"’ as the set of punctuation, as delimiter, for tokenization.

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <map>
#include <unordered_map>


std::unordered_map<std::string, int> simple_tokenizer(std::vector<std::string> &text, std::vector<bool> &is_punct) {
    std::unordered_map<std::string, int> word_freq;
    std::string token;
    for (int i = 0; i < text.size(); i++) {
        for (int j = 0; j < text[i].size(); j++) {
            if (text[i][j] < 0 || text[i][j] > 127) {
                continue;
            }
            if (is_punct[text[i][j]]) {
                if (!token.empty()) {
                    word_freq[token] += 1;
                    token.clear();
                }
            } else {
                token.push_back(text[i][j]);
            }
        }
        if (!token.empty()) {
            word_freq[token] += 1;
            token.clear();
        }
    }
    return word_freq;
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

// read text from file
int main(int argc, char* argv[]) {

    // take file name as input
    std::string file_name;
    file_name = argv[1];

    std::string dict_file_name = argv[2];

    std::ifstream file(file_name);
    std::vector<std::string> text;
    
    // read jsonlines file and store the text in a vector
    std::set<std::string> req_keys;
    req_keys.insert("title");
    req_keys.insert("abstract");
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> res = extract_val_from_json(req_keys, line);
        text.emplace_back(res[0]);
        text.emplace_back(res[1]);
    }
    file.close();

    // set of punctuation
    std::vector<bool> is_punct(128, false);
    is_punct[' '] = true;
    is_punct[','] = true;
    is_punct['.'] = true;
    is_punct[':'] = true;
    is_punct[';'] = true;
    is_punct['\"'] = true;
    is_punct['\''] = true;

    std::unordered_map<std::string, int> word_freq = simple_tokenizer(text, is_punct);

    std::ofstream vocab_file(dict_file_name);
    // first line 0 denoting simple tokenizer
    vocab_file << "0\n";
    for (auto it = word_freq.begin(); it != word_freq.end(); it++) {
        vocab_file << it->first<< "\n";
    }
    vocab_file.close();


}
