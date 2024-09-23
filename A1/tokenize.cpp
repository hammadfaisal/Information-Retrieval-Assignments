#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <fstream>

// split a sentence into words 
std::vector<std::string> split(std::string &text, std::vector<bool> &is_punct) {
    std::vector<std::string> words;
    std::string token;
    for (int i = 0; i < text.size(); i++) {
        if (text[i] < 0 || text[i] > 127) {
            continue;
        }
        if (is_punct[text[i]]) {
            if (!token.empty()) {
                words.push_back(token);
                token.clear();
            }
        } else {
            token.push_back(text[i]);
        }
    
    }

    if (!token.empty()) {
        words.push_back(token);
        token.clear();
    }
    return words;
}

void readSimpleDict(std::string &dictpath, std::vector<std::string> &vocab, std::map<std::string,int> &vocab_map) {
    // populate vocab and word_freq from dictpath
    std::ifstream file(dictpath);
    std::string line;
    //skip first line
    std::getline(file, line);

    while (std::getline(file, line)) {
        if (line.size() == 0) {
            continue;
        }
        vocab.push_back(line);
        vocab_map[line] = vocab.size()-1;
    }
    file.close();
    vocab.push_back("UNK");
    vocab_map["UNK"] = vocab.size()-1;
}

// UNK is vocab.size()
std::vector<int> simpleTokenizeSentence(std::string &text, std::map<std::string,int> &vocab_map, std::vector<bool> &is_punct) {
    std::vector<int> res;
    std::vector<std::string> words = split(text, is_punct);
    for (int i = 0; i < words.size(); i++) {
        if (vocab_map.find(words[i]) == vocab_map.end()) {
            res.push_back(vocab_map["UNK"]);
        } else {
            res.push_back(vocab_map[words[i]]);
        }
    }
    return res;
}

// replace 'Ġ' with ' '
void readBpeDict(std::string &dictpath, std::vector<std::string> &vocab, std::map<long long,int> &merges, std::map<std::string,int> &vocab_map) {
    const long long TOK_PAIR_SHIFT = 1'000'000'000;
    // populate vocab and merges from dictpath
    std::ifstream file(dictpath);
    std::string line;   
    //skip first line
    std::getline(file, line);
    // size of 'Ġ' 
    int wslen = std::string("Ġ").size();
    while (std::getline(file, line)) {
        if (line.size()==0) {
            continue;
        }
        if (line.find(',') != std::string::npos) {
            std::string tok1 = line.substr(0, line.find(','));
            if (tok1[0] < 0) {
                tok1 = " " + tok1.substr(wslen);
            }
            std::string tok2 = line.substr(line.find(',')+1);
            int tok1_idx = vocab_map[tok1];
            int tok2_idx = vocab_map[tok2];
            merges[(tok1_idx*TOK_PAIR_SHIFT)+tok2_idx] = vocab_map[tok1+tok2];
        } else {
            if (line[0] < 0) {
                line = " " + line.substr(wslen);
            } 
            vocab.push_back(line);
            vocab_map[line] = vocab.size()-1;
        }
    }
    file.close();
}


// merges -> key: token pair, value: index of the merged token
// space in string should be represented as " "
std::vector<int> bpeTokenizeWord(std::string &text, std::map<long long,int> &merges, std::map<std::string,int> &vocab_map) {
    const long long TOK_PAIR_SHIFT = 1'000'000'000;
    std::set<std::pair<int,int>> possible_merge; // pair of priority, merge index left
    std::vector<std::vector<int>> encoding(text.size(), std::vector<int>(3));
    for (int i = 0; i < text.size(); i++) {
        encoding[i][0] = vocab_map[std::string(1, text[i])];
        encoding[i][1] = i-1;
        encoding[i][2] = i+1;
        if (i>0) {
            long long tok_pair = (encoding[i-1][0] * TOK_PAIR_SHIFT) + encoding[i][0];
            if (merges.find(tok_pair) != merges.end()) {
                possible_merge.insert(std::make_pair(merges[tok_pair], i-1));
            }
        }
    }
    
    while(!possible_merge.empty()) {
        auto it = possible_merge.begin();
        int merge_idx = it->second;
        int merge_idx_2 = encoding[merge_idx][2];
        int merged_token = it->first;
        // remove old left and right merges and add new merge
        if (encoding[merge_idx][1] >= 0) {
            long long tok_pair = (encoding[encoding[merge_idx][1]][0] * TOK_PAIR_SHIFT) + encoding[merge_idx][0];
            if (merges.find(tok_pair) != merges.end()) {
                possible_merge.erase(std::make_pair(merges[tok_pair], encoding[merge_idx][1]));
            }
            tok_pair = (encoding[encoding[merge_idx][1]][0] * TOK_PAIR_SHIFT) + merged_token;
            if (merges.find(tok_pair) != merges.end()) {
                possible_merge.insert(std::make_pair(merges[tok_pair], encoding[merge_idx][1]));
            }

        }
        if (encoding[merge_idx_2][2] < text.size()) {
            long long tok_pair = (encoding[merge_idx_2][0] * TOK_PAIR_SHIFT) + encoding[encoding[merge_idx_2][2]][0];
            if (merges.find(tok_pair) != merges.end()) {
                possible_merge.erase(std::make_pair(merges[tok_pair], merge_idx_2));
            }
            tok_pair = (merged_token * TOK_PAIR_SHIFT) + encoding[encoding[merge_idx_2][2]][0];
            if (merges.find(tok_pair) != merges.end()) {
                possible_merge.insert(std::make_pair(merges[tok_pair], merge_idx));
            }
        }
        encoding[merge_idx][0] = merged_token;
        encoding[merge_idx][2] = encoding[merge_idx_2][2];
        if (encoding[merge_idx][2] < text.size()) {
            encoding[encoding[merge_idx][2]][1] = merge_idx;
        }
        possible_merge.erase(it);
    }
    std::vector<int> res;
    for (int i = 0; i < text.size(); i++) {
        res.push_back(encoding[i][0]);
        i = encoding[i][2]-1;
    }
    return res;
}

std::vector<int> bpeTokenizeSentence(std::string &text, std::map<long long,int> &merges, std::map<std::string,int> vocab_map ,std::vector<bool> &is_punct) {
    std::vector<int> res;
    std::vector<std::string> words = split(text, is_punct);

    for (int i = 0; i < words.size(); i++) {
        words[i] = " " + words[i];
        std::vector<int> word_encoding = bpeTokenizeWord(words[i], merges, vocab_map);
        res.insert(res.end(), word_encoding.begin(), word_encoding.end());
    }
    return res;
}

struct trieNode {
    int idx;
    std::unordered_map<char, trieNode*> children;
    trieNode() {
        idx = -1;
    }
};

struct trie {
    trieNode *root;
    trie() {
        root = new trieNode();
    }
    void insert(std::string &word, int idx) {
        trieNode *cur = root;
        for (int i = 0; i < word.size(); i++) {
            if (cur->children.find(word[i]) == cur->children.end()) {
                cur->children[word[i]] = new trieNode();
            }
            cur = cur->children[word[i]];
        }
        cur->idx = idx;
    }

    // return length of the longest prefix of word that is in the trie and the index of the word in the vocab
    std::pair<int,int> search(std::string &word, int offset) {
        trieNode *cur = root;
        int i = 0;
        int best_idx = -1;
        int best_len = 0;
        for (i = offset; i < word.size(); i++) {
            if (cur->children.find(word[i]) == cur->children.end()) {
                break;
            }
            cur = cur->children[word[i]];
            if (cur->idx != -1) {
                best_idx = cur->idx;
                best_len = i+1;
            }
        }
        return std::make_pair(best_len, best_idx);
        
    }
};

void readWordPieceDict(std::string &dictpath, std::vector<std::string> &vocab, trie &vocab_trie) {
    const long long TOK_PAIR_SHIFT = 1'000'000'000;
    // populate vocab and merges from dictpath
    std::ifstream file(dictpath);
    std::string line;   
    //skip first line
    std::getline(file, line);
    while (std::getline(file, line)) {
        if (line.size() == 0) {
            continue;
        }
        if (line.size()>=2 && line[0] == '#' && line[1] == '#') {
            line = line.substr(2);
        }
        else {
            line = " " + line;
        }
        if (line.size()==0) {
            continue;
        }
        vocab.push_back(line);
        

        vocab_trie.insert(line, vocab.size()-1);
    }
    file.close();
}

std::vector<int> wordPieceTokenizeWord(std::string &text, trie &vocab_trie) {
    std::vector<int> res;

    int i = 0;
    while (i < text.size()) {
        std::pair<int,int> match = vocab_trie.search(text, i);
        res.push_back(match.second);
        i = match.first;
    }
    return res;
}

std::vector<int> wordPieceTokenizeSentence(std::string &text, trie &vocab_trie, std::vector<bool> &is_punct) {
    std::vector<int> res;
    std::vector<std::string> words = split(text, is_punct);
    for (int i = 0; i < words.size(); i++) {
        words[i] = " " + words[i];
        std::vector<int> word_encoding = wordPieceTokenizeWord(words[i], vocab_trie);
        res.insert(res.end(), word_encoding.begin(), word_encoding.end());
    }
    return res;
}

