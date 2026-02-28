#include "commonlibs/segment_tree.hpp"
#include <gtest/gtest.h>

// ---- helpers ---------------------------------------------------------------

// Build a tree and call initialize(0, 0, size-1) — the canonical root call.
static commonlibs::segments_tree make_tree(const int *a, int n)
{
    commonlibs::segments_tree t(a, n);
    if (n > 0)
        t.initialize(0, 0, n - 1);
    return t;   // RVO / move; segments_tree has no copy, so the compiler will elide
}

// query returns the *index* into the original array of the minimum element.
// Wrap it so tests read naturally.
static int min_index(commonlibs::segments_tree &t, int i, int j)
{
    return t.query(0, i, j);
}

// ---- tests -----------------------------------------------------------------

TEST(SegmentTree, SingleElement)
{
    int a[] = {42};
    commonlibs::segments_tree t(a, 1);
    t.initialize(0, 0, 0);

    EXPECT_EQ(0, min_index(t, 0, 0));
}

TEST(SegmentTree, TwoElements_FirstSmaller)
{
    int a[] = {1, 5};
    commonlibs::segments_tree t(a, 2);
    t.initialize(0, 0, 1);

    EXPECT_EQ(0, min_index(t, 0, 1));   // index 0 holds the minimum (1)
    EXPECT_EQ(0, min_index(t, 0, 0));
    EXPECT_EQ(1, min_index(t, 1, 1));
}

TEST(SegmentTree, TwoElements_SecondSmaller)
{
    int a[] = {5, 1};
    commonlibs::segments_tree t(a, 2);
    t.initialize(0, 0, 1);

    EXPECT_EQ(1, min_index(t, 0, 1));
}

TEST(SegmentTree, AllSameValues)
{
    int a[] = {3, 3, 3, 3};
    commonlibs::segments_tree t(a, 4);
    t.initialize(0, 0, 3);

    // Any valid index in range is acceptable
    int idx = min_index(t, 0, 3);
    EXPECT_GE(idx, 0);
    EXPECT_LE(idx, 3);
    EXPECT_EQ(3, a[idx]);
}

TEST(SegmentTree, MinimumIsAtBeginning)
{
    int a[] = {1, 9, 7, 3, 5};
    commonlibs::segments_tree t(a, 5);
    t.initialize(0, 0, 4);

    EXPECT_EQ(0, min_index(t, 0, 4));
    EXPECT_EQ(0, min_index(t, 0, 2));
}

TEST(SegmentTree, MinimumIsAtEnd)
{
    int a[] = {9, 7, 3, 5, 1};
    commonlibs::segments_tree t(a, 5);
    t.initialize(0, 0, 4);

    EXPECT_EQ(4, min_index(t, 0, 4));
    EXPECT_EQ(4, min_index(t, 3, 4));
}

TEST(SegmentTree, MinimumIsInMiddle)
{
    int a[] = {5, 3, 1, 4, 9};
    commonlibs::segments_tree t(a, 5);
    t.initialize(0, 0, 4);

    EXPECT_EQ(2, min_index(t, 0, 4));
    EXPECT_EQ(2, min_index(t, 1, 3));
    EXPECT_EQ(2, min_index(t, 1, 2));  // a[1]=3, a[2]=1 — minimum is at index 2
}

TEST(SegmentTree, FullRangeQuery)
{
    int a[] = {4, 2, 7, 1, 8, 3};
    commonlibs::segments_tree t(a, 6);
    t.initialize(0, 0, 5);

    // Global minimum is 1 at index 3
    EXPECT_EQ(3, min_index(t, 0, 5));
}

TEST(SegmentTree, SubrangeQueries)
{
    int a[] = {4, 2, 7, 1, 8, 3};
    commonlibs::segments_tree t(a, 6);
    t.initialize(0, 0, 5);

    EXPECT_EQ(1, min_index(t, 0, 2));  // {4,2,7} → min=2 at index 1
    EXPECT_EQ(3, min_index(t, 2, 4));  // {7,1,8} → min=1 at index 3
    EXPECT_EQ(5, min_index(t, 4, 5));  // {8,3}   → min=3 at index 5
    EXPECT_EQ(5, min_index(t, 5, 5));  // single element
}

TEST(SegmentTree, NegativeValues)
{
    int a[] = {-3, 5, -1, 0, -7, 2};
    commonlibs::segments_tree t(a, 6);
    t.initialize(0, 0, 5);

    EXPECT_EQ(4, min_index(t, 0, 5));  // -7 at index 4 is minimum
    EXPECT_EQ(0, min_index(t, 0, 2));  // -3 at index 0
}

TEST(SegmentTree, OutOfRangeQueryReturnsMinusOne)
{
    int a[] = {1, 2, 3};
    commonlibs::segments_tree t(a, 3);
    t.initialize(0, 0, 2);

    // i > end of array — implementation returns -1 for no-overlap
    EXPECT_EQ(-1, min_index(t, 3, 5));
}

TEST(SegmentTree, LargerArray)
{
    // 16 elements; minimum is 0 at index 7
    int a[16];
    for (int i = 0; i < 16; ++i) a[i] = 16 - i;
    a[7] = 0;

    commonlibs::segments_tree t(a, 16);
    t.initialize(0, 0, 15);

    EXPECT_EQ(7, min_index(t, 0, 15));
    EXPECT_EQ(7, min_index(t, 5, 10));
}

// ---- additional edge cases / patterns ---------------------------------------

TEST(SegmentTree, DescendingArray)
{
    int a[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    auto t = make_tree(a, 10);
    // Minimum is 1 at index 9
    EXPECT_EQ(9, min_index(t, 0, 9));
    // Subrange [0,4]: minimum is 6 at index 4
    EXPECT_EQ(4, min_index(t, 0, 4));
}

TEST(SegmentTree, AscendingArray)
{
    int a[] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto t = make_tree(a, 8);
    // Full range: minimum is 1 at index 0
    EXPECT_EQ(0, min_index(t, 0, 7));
    // Subrange [4,7]: minimum is 5 at index 4
    EXPECT_EQ(4, min_index(t, 4, 7));
}

TEST(SegmentTree, PowerOfTwoSize)
{
    // Power-of-2 arrays often expose off-by-one bugs
    int a[] = {8, 3, 5, 1, 7, 2, 6, 4};
    auto t = make_tree(a, 8);
    EXPECT_EQ(3, min_index(t, 0, 7));  // min=1 at index 3
    EXPECT_EQ(5, min_index(t, 4, 7));  // min=2 at index 5
    EXPECT_EQ(3, min_index(t, 0, 3));  // min=1 at index 3
}

TEST(SegmentTree, AdjacentPointQueries)
{
    int a[] = {5, 1, 3, 2, 4};
    auto t = make_tree(a, 5);
    // Single-element queries
    EXPECT_EQ(0, min_index(t, 0, 0));  // 5
    EXPECT_EQ(1, min_index(t, 1, 1));  // 1
    EXPECT_EQ(2, min_index(t, 2, 2));  // 3
    EXPECT_EQ(3, min_index(t, 3, 3));  // 2
    EXPECT_EQ(4, min_index(t, 4, 4));  // 4
    // Adjacent two-element queries
    EXPECT_EQ(1, min_index(t, 0, 1));  // min(5,1) at 1
    EXPECT_EQ(1, min_index(t, 1, 2));  // min(1,3) at 1
    EXPECT_EQ(3, min_index(t, 2, 3));  // min(3,2) at 3
    EXPECT_EQ(3, min_index(t, 3, 4));  // min(2,4) at 3
}

TEST(SegmentTree, LargeArray_64Elements)
{
    int a[64];
    for (int i = 0; i < 64; ++i) a[i] = 100;
    a[42] = -1;   // minimum at index 42

    auto t = make_tree(a, 64);
    EXPECT_EQ(42, min_index(t, 0, 63));
    EXPECT_EQ(42, min_index(t, 40, 50));
    // In a range not containing 42, all values are 100
    int idx = min_index(t, 0, 10);
    EXPECT_GE(idx, 0);
    EXPECT_LE(idx, 10);
    EXPECT_EQ(100, a[idx]);
}
