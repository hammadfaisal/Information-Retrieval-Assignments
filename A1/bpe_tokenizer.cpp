#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <set>
#include <map>




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
                    word_freq[" "+token] += 1;
                    token.clear();
                }
            } else {
                token.push_back(text[i][j]);
            }
        }
        if (!token.empty()) {
            word_freq[" "+token] += 1;
            token.clear();
        }
    }
    return word_freq;
}

void bpe_tokenizer(std::vector<std::string> &text, std::vector<bool> &is_punct,int vocab_size, std::string &dict_file_name) {
    if (vocab_size > 1'000'000'000) {
        std::cerr << "Vocab size should be less than 1000000000\n";
        exit(1);
    }

    // initial vocab contains all ascii characters
    std::vector<std::string> vocab;
    for (int i = 0; i < 128; i++) {
        vocab.push_back(std::string(1, (char)i));
    }

    std::unordered_map<std::string, int> word_freq = simple_tokenizer(text, is_punct);
    
    // not really sorted, name misleading
    std::vector<std::pair<int, std::string>> sorted_words;
    for (auto it = word_freq.begin(); it != word_freq.end(); it++) {
        sorted_words.push_back(std::make_pair(it->second, it->first));
    }
    // assuming total count of all words is less than 2^31

    const long long TOK_PAIR_SHIFT = 1'000'000'000;
    
    std::unordered_map<int, std::set<int>> tok_in_words; // map from a token to the words that contain the token
    std::vector<std::vector<int>> encoding; // encoding of the words
    std::unordered_map<long long, long long> tok_pair_cur_freq; // frequency of a pair of tokens
    std::priority_queue<std::pair<long long, long long>> pq; // priority queue to store the frequency of token pairs

    // initialize tok_pair_cur_freq
    for (int i = 0; i < sorted_words.size(); i++) {
        std::string word = sorted_words[i].second;
        if (word.size() == 1) {
            encoding.push_back({1, word[0]});
            continue;    
        }
        int tok1 = word[0];
        std::vector<int> cur_encoding;
        cur_encoding.push_back(0);
        cur_encoding.push_back(tok1);
        tok_in_words[tok1].insert(i);

        for (int j = 1; j < word.size(); j++) {
            int tok2 = word[j];
            cur_encoding.push_back(tok2);
            long long tok_pair = tok1 * TOK_PAIR_SHIFT + tok2;
            tok_pair_cur_freq[tok_pair]+=sorted_words[i].first;
            tok_in_words[tok2].insert(i);
            tok1 = tok2;
        }
        cur_encoding[0] = cur_encoding.size();
        encoding.push_back(cur_encoding);
    }

    // initialize priority queue
    for (auto it = tok_pair_cur_freq.begin(); it != tok_pair_cur_freq.end(); it++) {
        pq.push(std::make_pair(it->second, it->first));
    }

    std::vector<std::pair<int,int>> merges;

    // merge tokens
    while (vocab.size() < vocab_size && !pq.empty()) {
        std::pair<long long, long long> top = pq.top();
        pq.pop();
        long long freq = top.first;
        long long tok_pair = top.second;

        int tok1 = tok_pair / TOK_PAIR_SHIFT;
        int tok2 = tok_pair % TOK_PAIR_SHIFT;
        if (tok_pair_cur_freq[tok_pair] != freq) {
            continue;
        }
        merges.push_back(std::make_pair(tok1, tok2));
        vocab.push_back(vocab[tok1] + vocab[tok2]);
        std::set<long long> update_in_pq;
        std::vector<int> tok_in_words_vec;
        // intersection
        for (auto it = tok_in_words[tok1].begin(); it != tok_in_words[tok1].end(); it++) {
            if (tok_in_words[tok2].find(*it) != tok_in_words[tok2].end()) {
                tok_in_words_vec.push_back(*it);
            }
        }
        for (auto it = tok_in_words_vec.begin(); it != tok_in_words_vec.end(); it++) {
            int word_idx = *it;
            
            bool tok1_remain = false;
            bool tok2_remain = false;
            bool pair_found = false;
            int shift = 0;
            for (int j = 2; j < encoding[word_idx][0]; j++) {
                if (encoding[word_idx][j-shift-1] == tok1 && encoding[word_idx][j] == tok2) {
                    pair_found = true;
                    if (j>=3) {
                        long long old_pair = encoding[word_idx][j-shift-2] * TOK_PAIR_SHIFT + tok1;
                        long long cur_pair = encoding[word_idx][j-shift-2] * TOK_PAIR_SHIFT + vocab.size() - 1;
                        tok_pair_cur_freq[old_pair] -= sorted_words[word_idx].first;
                        tok_pair_cur_freq[cur_pair] += sorted_words[word_idx].first;
                        update_in_pq.insert(old_pair);
                        update_in_pq.insert(cur_pair);
                    }
                    if (j+1 < encoding[word_idx][0]) {
                        long long old_pair = tok2 * TOK_PAIR_SHIFT + encoding[word_idx][j+1];
                        long long cur_pair = (vocab.size() - 1) * TOK_PAIR_SHIFT + encoding[word_idx][j+1];
                        tok_pair_cur_freq[old_pair] -= sorted_words[word_idx].first;
                        tok_pair_cur_freq[cur_pair] += sorted_words[word_idx].first;
                        update_in_pq.insert(old_pair);
                        update_in_pq.insert(cur_pair);
                    }
                    shift += 1;
                    encoding[word_idx][j-shift] = vocab.size() - 1;
                }
                else {
                    if (encoding[word_idx][j-shift-1] == tok1) {
                        tok1_remain = true;
                    }
                    if (encoding[word_idx][j] == tok2) {
                        tok2_remain = true;
                    }
                    encoding[word_idx][j-shift] = encoding[word_idx][j];
                }
            }
            encoding[word_idx][0] -= shift;
            if (!tok1_remain) {
                tok_in_words[tok1].erase(word_idx);
            }
            if (!tok2_remain) {
                tok_in_words[tok2].erase(word_idx);
            }
            if (pair_found) {
                tok_in_words[vocab.size()-1].insert(word_idx);
            }
        }
        for (auto it = update_in_pq.begin(); it != update_in_pq.end(); it++) {
            pq.push(std::make_pair(tok_pair_cur_freq[*it], *it));
        }
        // set count of current tok_pair to -1 so that it is not considered again
        tok_pair_cur_freq[tok_pair] = -1;
    }

    // write merges and vocab to file
    std::ofstream merge_file(dict_file_name);
    // first line 1 denoting bpe tokenizer
    merge_file << "1\n";
    for (int i = 0; i < vocab.size(); i++) {
        if (vocab[i][0] == ' ') {
            vocab[i] = "Ġ" + vocab[i].substr(1);
        }
        merge_file << vocab[i] << "\n";
    }
    for (int i = 0; i < merges.size(); i++) {
        merge_file << vocab[merges[i].first] << "," << vocab[merges[i].second] << "\n";
    }
    merge_file.close();
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

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <file_name> <vocab_size> <dict_file_name>\n";
        exit(1);
    }
    std::string file_name = argv[1];
    int vocab_size = std::stoi(argv[2]);
    std::string dict_file_name = argv[3];
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
    int tot_len = text.size();

    // choose first 50% of the text
    while (text.size() > tot_len/2) {
        text.pop_back();
    }

    // set of punctuation
    std::vector<bool> is_punct(128, false);
    is_punct[' '] = true;
    is_punct[','] = true;
    is_punct['.'] = true;
    is_punct[':'] = true;
    is_punct[';'] = true;
    is_punct['\"'] = true;
    is_punct['\''] = true;

    bpe_tokenizer(text, is_punct, vocab_size, dict_file_name);
    
}