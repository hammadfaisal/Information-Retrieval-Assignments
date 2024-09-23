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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <docs_file_path> <cleaned_docs_file_path>" << std::endl;
        return 1;
    }

    std::string docs_file_path = argv[1];
    std::string cleaned_docs_file_path = argv[2];
    std::set<std::string> done_docs;
    std::ifstream docs_file(docs_file_path);
    std::string line;
    std::ofstream out_file(cleaned_docs_file_path);
    std::set<std::string> req_keys;
    req_keys.insert("doc_id");
    while (std::getline(docs_file, line)) {
        std::vector<std::string> vals = extract_val_from_json(req_keys, line);
        if (vals.size() > 0) {
            if (done_docs.find(vals[0]) == done_docs.end()) {
                out_file << line << std::endl;
                done_docs.insert(vals[0]);
            }
        }
    }
    
    
}