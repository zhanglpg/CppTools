#pragma once
#include <cctype>
#include <iostream>
#include <memory>

namespace commonlibs {

struct Vertex {
    int words    = 0;
    int prefixes = 0;
    enum { num_leaves = 26 };
    // unique_ptr replaces the raw pointer array + manual destructor.
    std::unique_ptr<Vertex> edges[num_leaves];

    Vertex() = default;
    // Destructor is compiler-generated: recursively destroys unique_ptr children.

    // Add a word (lowercase alphabet only).  Returns the word count at root.
    int addWord(const char *p_word, std::size_t size) {
        if (p_word == nullptr || size == 0) return words;

        Vertex *cur = this;
        for (std::size_t k = 0; k < size; ++k) {
            const char ch = p_word[k];
            if (!::isalpha(static_cast<unsigned char>(ch))) {
                std::cerr << "only alphabets are allowed\n";
                return words;
            }
            const int idx = ::tolower(static_cast<unsigned char>(ch)) - 'a';
            if (!cur->edges[idx]) {
                cur->edges[idx] = std::make_unique<Vertex>();
                cur->prefixes++;
            }
            cur = cur->edges[idx].get();
        }
        cur->words++;
        return words;
    }

    // Count exact occurrences of p_word.
    int countWords(const char *p_word, std::size_t size) {
        if (p_word == nullptr || size == 0) return words;

        const Vertex *cur = this;
        for (std::size_t k = 0; k < size; ++k) {
            const char ch = p_word[k];
            if (!::isalpha(static_cast<unsigned char>(ch))) {
                std::cerr << "only alphabets are allowed\n";
                return words;
            }
            const int idx = ::tolower(static_cast<unsigned char>(ch)) - 'a';
            if (!cur->edges[idx]) return 0;
            cur = cur->edges[idx].get();
        }
        return cur->words;
    }

    // Count how many unique children the node reached by p_word has.
    int countPrefixes(const char *p_word, std::size_t size) {
        if (p_word == nullptr || size == 0) return prefixes;

        const Vertex *cur = this;
        for (std::size_t k = 0; k < size; ++k) {
            const char ch = p_word[k];
            if (!::isalpha(static_cast<unsigned char>(ch))) {
                std::cerr << "only alphabets are allowed\n";
                return 0;
            }
            const int idx = ::tolower(static_cast<unsigned char>(ch)) - 'a';
            if (!cur->edges[idx]) return 0;
            cur = cur->edges[idx].get();
        }
        return cur->prefixes;
    }
};

class traverser_tree {
public:
    void traverser_recursive(Vertex *root) {
        traverser_recursive_imp(root, -1, 0);
    }
private:
    void traverser_recursive_imp(const Vertex *root, int k, int level) {
        if (!root) return;
        if (k >= 0) {
            for (int i = 0; i < level; ++i) std::cout << ' ';
            std::cout << static_cast<char>('a' + k)
                      << "( " << root->words << "," << root->prefixes << ")\n";
        }
        for (int i = 0; i < 26; ++i)
            traverser_recursive_imp(root->edges[i].get(), i, level + 1);
    }
};

} // namespace commonlibs
