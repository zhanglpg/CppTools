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
