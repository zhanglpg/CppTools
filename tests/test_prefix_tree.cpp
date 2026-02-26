#include "commonlibs/prefix_tree.hpp"
#include <gtest/gtest.h>
#include <cstring>

// Helper: add a word to a root Vertex
static void add(commonlibs::Vertex &root, const char *word)
{
    root.addWord(word, std::strlen(word));
}

// Helper: count exact word matches
static int countW(commonlibs::Vertex &root, const char *word)
{
    return root.countWords(word, std::strlen(word));
}

// Helper: count prefix matches
static int countP(commonlibs::Vertex &root, const char *prefix)
{
    return root.countPrefixes(prefix, std::strlen(prefix));
}

// ---- addWord ---------------------------------------------------------------

TEST(PrefixTree, AddSingleWord_CountIsOne)
{
    commonlibs::Vertex root;
    add(root, "hello");
    EXPECT_EQ(1, countW(root, "hello"));
}

TEST(PrefixTree, AddWordTwice_CountIsTwo)
{
    commonlibs::Vertex root;
    add(root, "hello");
    add(root, "hello");
    EXPECT_EQ(2, countW(root, "hello"));
}

TEST(PrefixTree, AddDifferentWords_CountsIndependent)
{
    commonlibs::Vertex root;
    add(root, "apple");
    add(root, "apply");
    add(root, "apt");
    EXPECT_EQ(1, countW(root, "apple"));
    EXPECT_EQ(1, countW(root, "apply"));
    EXPECT_EQ(1, countW(root, "apt"));
}

TEST(PrefixTree, MissingWord_CountIsZero)
{
    commonlibs::Vertex root;
    add(root, "hello");
    EXPECT_EQ(0, countW(root, "world"));
    EXPECT_EQ(0, countW(root, "hell"));   // prefix only, not full word
}

TEST(PrefixTree, CaseInsensitive)
{
    commonlibs::Vertex root;
    add(root, "Hello");
    // addWord lowercases, so "hello" and "Hello" should map to the same node
    EXPECT_EQ(1, countW(root, "hello"));
    EXPECT_EQ(1, countW(root, "Hello"));
}

TEST(PrefixTree, EmptyWordReturnsZeroWithoutCrash)
{
    commonlibs::Vertex root;
    // size == 0 → addWord early-returns; should not crash
    root.addWord("anything", 0);
    root.addWord(nullptr, 0);
    EXPECT_EQ(0, countW(root, "anything"));
}

// ---- countPrefixes ---------------------------------------------------------

TEST(PrefixTree, PrefixCount_BasicSharing)
{
    commonlibs::Vertex root;
    add(root, "apple");
    add(root, "apply");
    add(root, "application");

    // "app" is a prefix of all three; the node at 'p' (third char) has
    // prefixes > 0.  We test that a known prefix returns nonzero.
    EXPECT_GT(countP(root, "app"), 0);
    EXPECT_GT(countP(root, "appl"), 0);
}

TEST(PrefixTree, PrefixCount_NonExistentPrefix)
{
    commonlibs::Vertex root;
    add(root, "hello");
    EXPECT_EQ(0, countP(root, "world"));
    EXPECT_EQ(0, countP(root, "z"));
}

TEST(PrefixTree, PrefixCount_EmptyPrefix)
{
    // Passing size 0 → returns root->prefixes (number of unique first-chars added)
    commonlibs::Vertex root;
    add(root, "apple");
    add(root, "banana");
    // root->prefixes == 2 (one for 'a', one for 'b')
    int p = root.countPrefixes("", 0);
    EXPECT_EQ(2, p);
}

// ---- edge cases ------------------------------------------------------------

TEST(PrefixTree, LongWord)
{
    commonlibs::Vertex root;
    const char *word = "abcdefghijklmnopqrstuvwxyz";
    add(root, word);
    EXPECT_EQ(1, countW(root, word));
}

TEST(PrefixTree, SingleLetterWord)
{
    commonlibs::Vertex root;
    add(root, "a");
    EXPECT_EQ(1, countW(root, "a"));
    EXPECT_EQ(0, countW(root, "b"));
}

TEST(PrefixTree, MultipleInsertionsOfSamePrefix)
{
    commonlibs::Vertex root;
    add(root, "cat");
    add(root, "car");
    add(root, "card");
    EXPECT_EQ(1, countW(root, "cat"));
    EXPECT_EQ(1, countW(root, "car"));
    EXPECT_EQ(1, countW(root, "card"));
    EXPECT_EQ(0, countW(root, "ca"));    // not inserted as complete word
    EXPECT_EQ(0, countW(root, "care"));  // not inserted
}
