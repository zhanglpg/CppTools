	
#ifndef __COMMONLIBS_SEGMENTS_TREE_HPP

#define __COMMONLIBS_SEGMENTS_TREE_HPP

#include <iostream> 
#include <cmath>

namespace commonlibs {
// segment tree is a tree that can be used to find range minimum 
class segments_tree {
public:
	segments_tree(const int *A_,  int arraysize_) {
		A = A_ ;
		array_size =arraysize_ ;
		msize = 0 ; 
		if(arraysize_ > 0)  {
			msize=   4 * arraysize_ ;
			M = new int[ msize] ;
			for(int k = 0 ; k < msize; ++k) {
				M[k] = -1 ;
			}
			
		}
	}

	void initialize(int node, int begin, int end) {
		if(node >= msize) {
			std::cerr << "Error, index " << node << " > msize =" << msize <<std::endl ;
		}
		if(begin == end) {
			M[node] = begin ;
		}
		else {
			initialize(  2* (node + 1) -1 , begin, (begin + end)/2) ;
			initialize(  2* (node + 1)  ,  (begin + end)/2+ 1 , end) ;
			if(A[M[2*(node+1)-1]] <= A[M[2*(node+1)]])
				M[node] = M[2*(node+1)-1] ;
			else
				M[node] = M[2*(node+1)] ;

		}
	}

	int query(int node, int i, int j) {
		return query_imp(node, 0, array_size - 1, i, j) ;
	}

	~segments_tree() {
		delete [] M ;
	} 

private:

	int query_imp(int node, int begin, int end, int i, int j) {
		if( i > end || j < begin) {
			return -1 ;
		}

		if(begin >= i && end <= j) {
			return M[node] ;
		}

		int left = query_imp( 2* (node+1)-1,  begin, (begin+end)/2 , i, j) ;
		int right =query_imp(2*(node+1) , (begin+end)/2 + 1, end, i, j) ;
		if(left == -1) 
			return right ;
		if(right == -1) 
			return left ;

		if(A[left]<= A[right])
			return left; 

		return right ;

	}

	const int *A ; // array ; 
	int array_size ; 
	int *M ;
	int msize ;

} ;
}

#endif 