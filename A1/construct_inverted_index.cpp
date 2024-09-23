#include <iostream>
#include <algorithm>
#include <cctype>
#include <queue>
#include <sstream>
#include <utility>
#include "tokenize.cpp"

void writePostingsList(const std::vector<std::vector<std::pair<std::string, int>>>& postings_list, const std::string& filename) {
    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        throw std::runtime_error("Unable to open file for writing.");
    }

    // Write the size of the outer vector
    size_t outer_size = postings_list.size();
    outfile.write(reinterpret_cast<const char*>(&outer_size), sizeof(outer_size));

    for (const auto& inner_vec : postings_list) {
        // Write the size of the inner vector
        size_t inner_size = inner_vec.size();
        outfile.write(reinterpret_cast<const char*>(&inner_size), sizeof(inner_size));

        for (const auto& pair : inner_vec) {
            // Write the size of the string
            size_t str_size = pair.first.size();
            outfile.write(reinterpret_cast<const char*>(&str_size), sizeof(str_size));
            // Write the string itself
            outfile.write(pair.first.data(), str_size);
            // Write the integer
            outfile.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
        }
    }

    outfile.close();
}

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
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <docs_file_path> <indexfile_name> <tokenizer_type>\n";
        exit(1);
    }

    std::string docs_file_path = argv[1];
    std::string indexfile_name = argv[2];
    int tok_type = std::stoi(argv[3]);
    std::string dict_file_name = indexfile_name+".dict";
    
    // set of punctuation
    std::vector<bool> is_punct(128, false);
    is_punct[' '] = true;
    is_punct[','] = true;
    is_punct['.'] = true;
    is_punct[':'] = true;
    is_punct[';'] = true;
    is_punct['\"'] = true;
    is_punct['\''] = true;

    std::set<std::string> req_keys;
    req_keys.insert("doc_id");
    req_keys.insert("title");
    req_keys.insert("abstract");

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



    const int MAX_SIZE = 40'000'000; // in bytes
    std::ifstream docs_file(docs_file_path);
    std::string line;
    std::vector<std::pair<int, std::string>> term_doc_pair;
    int buffers_written = 0;
    int curr_size = 0;
    while (std::getline(docs_file, line)) {
        std::vector<std::string> vals = extract_val_from_json(req_keys, line);
        std::vector<int> tokens;
        std::string text = vals[1] + " " + vals[2];
        if (tok_type == 0) {
            tokens = simpleTokenizeSentence(text, vocab_map, is_punct);
        }
        else if (tok_type == 1) {
            tokens = bpeTokenizeSentence(text, merges, vocab_map, is_punct);
        }
        else if (tok_type == 2) {
            tokens = wordPieceTokenizeSentence(text, vocab_trie, is_punct);
        }


        
        curr_size += (sizeof(vals[0]) + sizeof(int))*(tokens.size());
        for (int i = 0; i < tokens.size(); i++) {
            term_doc_pair.push_back({tokens[i], vals[0]});
        }
        if (curr_size > MAX_SIZE) {
            std::sort(term_doc_pair.begin(), term_doc_pair.end());
            std::vector<std::vector<std::pair<std::string,int>>> postings_list(vocab.size(), std::vector<std::pair<std::string,int>>());
            for (int i = 0; i < term_doc_pair.size(); i++) {
                int term_id = term_doc_pair[i].first;
                std::string doc_id = term_doc_pair[i].second;
                // std::cout << "term_id: " << term_id << " doc_id: " << doc_id << std::endl;
                if (postings_list[term_id].empty() || postings_list[term_id].back().first != doc_id) {
                    postings_list[term_id].push_back({doc_id, 1});
                } else {
                    postings_list[term_id].back().second++;
                }
            }
            // write to file and update buffer_term_start_end_pointers
            std::string filename = "temp_inverted_index_" + std::to_string(buffers_written) + ".idx";
            writePostingsList(postings_list, filename);
            buffers_written++;
            curr_size = 0;
            term_doc_pair.clear();
        }
    }
    if (curr_size) {
        std::sort(term_doc_pair.begin(), term_doc_pair.end());
        // create inverted index
        std::vector<std::vector<std::pair<std::string,int>>> postings_list(vocab.size(), std::vector<std::pair<std::string,int>>());
        for (int i = 0; i < term_doc_pair.size(); i++) {
            int term_id = term_doc_pair[i].first;
            std::string doc_id = term_doc_pair[i].second;
            if (postings_list[term_id].empty() || postings_list[term_id].back().first != doc_id) {
                postings_list[term_id].push_back({doc_id, 1});
            } else {
                postings_list[term_id].back().second++;
            }
        }
        // write to file and update buffer_term_start_end_pointers
        std::string filename = "temp_inverted_index_" + std::to_string(buffers_written) + ".idx";
        writePostingsList(postings_list, filename);
        buffers_written++;
        curr_size = 0;
        term_doc_pair.clear();
    }

    std::ofstream combined_outfile(indexfile_name+".idx", std::ios::binary);
    size_t outer_size = vocab.size();
    combined_outfile.write(reinterpret_cast<const char*>(&outer_size), sizeof(outer_size));

    std::vector<std::ifstream> temp_inverted_index_files;
    for (int i = 0; i < buffers_written; i++) {
        temp_inverted_index_files.push_back(std::ifstream("temp_inverted_index_" + std::to_string(i) + ".idx", std::ios::binary));
        // read and discard the size of the outer vector
        size_t outer_size;
        temp_inverted_index_files[i].read(reinterpret_cast<char*>(&outer_size), sizeof(outer_size));
    }

    // merge the inverted index files. make it term by term. only data of one term is in memory at a time
    for (int i = 0; i < vocab.size(); i++) {
        std::vector<std::pair<std::string, int>> term_doc_freq;

        for (int j = 0; j < buffers_written; j++) {
            size_t inner_size;
            temp_inverted_index_files[j].read(reinterpret_cast<char*>(&inner_size), sizeof(inner_size));
            std::vector<std::pair<std::string, int>> inner_vec(inner_size);
            for (auto &pair : inner_vec) {
                size_t str_size;
                temp_inverted_index_files[j].read(reinterpret_cast<char*>(&str_size), sizeof(str_size));
                std::string str(str_size, '\0');
                temp_inverted_index_files[j].read(&str[0], str_size);
                int value;
                temp_inverted_index_files[j].read(reinterpret_cast<char*>(&value), sizeof(value));
                pair = std::make_pair(std::move(str), value);
            }
            term_doc_freq.insert(term_doc_freq.end(), inner_vec.begin(), inner_vec.end());
        }
        // write term_doc_freq to combined_outfile
        size_t inner_size = term_doc_freq.size();
        combined_outfile.write(reinterpret_cast<const char*>(&inner_size), sizeof(inner_size));
        for (const auto& pair : term_doc_freq) {
            size_t str_size = pair.first.size();
            combined_outfile.write(reinterpret_cast<const char*>(&str_size), sizeof(str_size));
            combined_outfile.write(pair.first.data(), str_size);
            combined_outfile.write(reinterpret_cast<const char*>(&pair.second), sizeof(pair.second));
        }
    }

    for (int i = 0; i < buffers_written; i++) {
        temp_inverted_index_files[i].close();
        std::string filename = "temp_inverted_index_" + std::to_string(i) + ".idx";
        std::remove(filename.c_str());
    }
    combined_outfile.close();

}