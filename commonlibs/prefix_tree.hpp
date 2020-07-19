#ifndef __PREFIX_COMMONLIBS_TREE_HPP

#define __PREFIX_COMMONLIBS_TREE_HPP

#include <cstdlib>
#include <cctype> 
#include <iostream>
#include <vector> 
namespace commonlibs {

	typedef struct Vertex {
		int words ;
		int prefixes ;
		enum { num_leaves = 26 } ; 
		Vertex *edges [num_leaves] ;
		Vertex *edges_refs [num_leaves] ;

		Vertex () {
			words = 0 ; 
			prefixes = 0 ;
			for(int i = 0 ;i < num_leaves ;++ i) {
				edges[i] = NULL ;
				edges_refs[i] = NULL ; 
			}
		} ;

		~Vertex () {
			for(int k = 0 ; k < num_leaves ; ++ k) {
				if(edges[k] != NULL) 
					delete edges[k] ;
			}
		}
		// add a word: p_word points to the word to be added, size is the length of the string. 
		// only lowercase alphabet is allowed. 
		// returns: Number of words
		int addWord(const char *p_word, size_t size) {
			if(p_word == NULL || size == 0) {
				return words ;
			}
			Vertex * cur = this ; 
			for(int k  = 0 ; k < size ; ++ k){ 
				const char ch = p_word[k] ;
				if(! ::isalpha(ch)) {
					std::cerr << "only alphbets are allowed" << std::endl ;
					return words; 
				}

				char lowerch = ::tolower(ch) ;
				if(cur->edges[(int) (lowerch - 'a')] == NULL) {
					cur->edges[(int) (lowerch - 'a')] = new Vertex() ;
					cur->prefixes ++ ;
				}
				cur = cur->edges[(int) (lowerch - 'a')] ;
			}
			cur->words ++ ;	

			return words ;
		} ;

		int countWords(const char *p_word, size_t size) {
			Vertex *cur = this ;

			if(p_word == NULL || size == 0) {
				return this->words ;
			}

			for(int k = 0 ; k < size ; ++ k) {
				const char ch = p_word[k] ;
				if(! ::isalpha(ch)) {
					std::cerr << "only alphbets are allowed" << std::endl ;
					return words; 
				}

				char lowerch = ::tolower(ch) ;

				if(cur->edges[(int) lowerch -'a'] ==  NULL) {
					return 0 ;//  no matched words
				}
				else {
					cur = cur ->edges[(int) lowerch -'a']  ;
				}
			
			}
			return cur->words ;
		}; 

		int countPrefixes(const char *p_word, size_t size) {
			Vertex *cur = this ;

			if(p_word == NULL || size == 0) {
				return this->prefixes ;
			}

			for(int k = 0 ; k < size ; ++ k) {
				const char ch = p_word[k] ;
				if(! ::isalpha(ch)) {
					std::cerr << "only alphbets are allowed" << std::endl ;
					return 0 ; 
				}

				char lowerch = ::tolower(ch) ;

				if(cur->edges[(int) lowerch -'a'] ==  NULL) {
					return 0 ;//  no matched words
				}
				else {
					cur = cur ->edges[(int) lowerch -'a']  ;
				}
			
			}
			return cur->prefixes ;
		}; 

	} Vertex ;



	typedef struct traverser_Vertex {
		Vertex * v ; 
		int  cindex ;
		traverser_Vertex (Vertex * v_) : v(v_), cindex(0) {} 
	} traverser_Vertex ;


	class traverser_tree {
	public: 
		void traverser_recursive(Vertex *root) {
			traverser_recursive_imp(root, -1, 0) ;
		}
	private: 
		void traverser_recursive_imp(Vertex * root, int k, int level) {
			if(root == NULL) {
				return  ;
			}
			if(k >= 0) {
				for(int i = 0 ; i < level ; ++ i) 
					std::cout << " " ; 
				std::cout <<  (char)((int) ('a') + k ) << "( " << root->words << "," << root->prefixes <<")" <<std::endl ;
			}
			for(int k = 0; k < 26; ++ k) {
				traverser_recursive_imp(root->edges[k], k, level + 1) ;
			}
		}
		std::vector<traverser_Vertex> v_t;
	} ;
}




#endif