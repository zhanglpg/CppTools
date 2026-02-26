#pragma once
#include <iostream>
#include <vector>

namespace commonlibs {

// Segment tree for range-minimum queries.
// Stores, for every node, the index into A of the minimum element in that range.
class segments_tree {
public:
    segments_tree(const int *A_, int arraysize_)
        : A(A_), array_size(arraysize_),
          M(arraysize_ > 0 ? 4 * arraysize_ : 0, -1)
    {}

    void initialize(int node, int begin, int end) {
        if (node >= static_cast<int>(M.size())) {
            std::cerr << "Error, index " << node
                      << " >= msize=" << M.size() << '\n';
            return;
        }
        if (begin == end) {
            M[node] = begin;
        } else {
            initialize(2 * (node + 1) - 1, begin, (begin + end) / 2);
            initialize(2 * (node + 1),     (begin + end) / 2 + 1, end);
            M[node] = (A[M[2*(node+1)-1]] <= A[M[2*(node+1)]])
                      ? M[2*(node+1)-1]
                      : M[2*(node+1)];
        }
    }

    int query(int node, int i, int j) {
        return query_imp(node, 0, array_size - 1, i, j);
    }

private:
    int query_imp(int node, int begin, int end, int i, int j) {
        if (i > end || j < begin) return -1;
        if (begin >= i && end <= j) return M[node];

        int left  = query_imp(2*(node+1)-1, begin,           (begin+end)/2,     i, j);
        int right = query_imp(2*(node+1),   (begin+end)/2+1, end,               i, j);
        if (left  == -1) return right;
        if (right == -1) return left;
        return (A[left] <= A[right]) ? left : right;
    }

    const int       *A;
    int              array_size;
    std::vector<int> M;
};

} // namespace commonlibs
