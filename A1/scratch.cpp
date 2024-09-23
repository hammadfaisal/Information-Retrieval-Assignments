#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <unordered_map>
#include <queue>
#include <set>
#include <utility>

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

// std::string removeNonAscii(const std::string& input) {
//     std::string result;
//     for (char c : input) {
//         if (static_cast<unsigned char>(c) < 128) {
//             result += c;
//         }
//     }
//     return result;
// }

// int main() {
//     std::string input;
//     std::cout << "Enter a UTF-8 string: ";
//     std::getline(std::cin, input);

//     std::string cleanedString = removeNonAscii(input);

//     std::cout << "Cleaned string (ASCII only): " << cleanedString << std::endl;

//     return 0;
// }

int main() {
    // Example vector
    std::vector<std::vector<std::pair<std::string, int>>> postings_list = {
        {{"apple", 1}, {"banana", 2}}, {},
        {{"cherry", 3}, {"date", 4}},
    };

    // Write to a binary file
    writePostingsList(postings_list, "postings_list.bin");

    // Read from the binary file
    auto loaded_postings_list = readPostingsList("postings_list.bin");

    // Verify the content
    for (const auto& inner_vec : loaded_postings_list) {
        std::cout << "size: " << inner_vec.size() << "\n";
        for (const auto& pair : inner_vec) {
            std::cout << pair.first << ": " << pair.second << "\n";
        }
    }

    return 0;
}