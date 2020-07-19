
#ifndef __COMMONLIBS_ALGORITHMS_HPP
#define __COMMONLIBS_ALGORITHMS_HPP


#include <vector> 
#include <iostream> 
#include <queue>

#include <algorithm> 


namespace commonlibs {

	
	typedef int distance_t ; 
	typedef int vertexid_t ;
	typedef  std::pair<distance_t, vertexid_t> gedge ;
	
	typedef  std::pair<int, int> ii ;
	//typedef std::pair<vertexid_t, vertexid_t>  gedge ;
	typedef std::vector< std::vector< gedge > > G_vvii ;


	class shortest_path_algo {
	public: 
		void initialize(const int totalvertex, const int fromvertex_id)  {
			Dsource = std::vector<int>(totalvertex, 987654321) ;
			Dsource[fromvertex_id] = 0 ;
			previous_vertex = std::vector<int>(totalvertex, -1) ;

			v_visited = std::vector<bool>(totalvertex, false) ; 

	//		Q.push(gedge(0, 0)) ;

			fid = fromvertex_id ;
		}

		// vvii as graph structure, 
		int Dijkstra(const G_vvii &graph, int fromid, int toid = -1) {
			int distance = -1 ;
			initialize(graph.size() , fromid) ;

			Dsource[fromid] = 0 ;
			Q.push(ii(0, fromid)) ;

			while(!Q.empty())  {
				ii e = Q.top() ;
				Q.pop() ;

				int distance = e.first ; 
				int vid = e.second ;

				if(vid == toid) 
					break ; // destination reached 

				// check if visited.  We don't need a visited flag array because priority queue guarantees that 
				///  overall distance  > D[v], because the edges are fetched in ascending order based on their length 
				if(distance <= Dsource[vid]) {
					for (int i = 0 ; i < graph[vid].size() ; ++ i) { 
						const gedge & eout = graph[vid][i] ;
						if(Dsource[vid] + eout.first < Dsource[eout.second]) // 
						{
							Dsource[eout.second] = Dsource[vid] + eout.first ;
							Q.push(ii(Dsource[eout.second], eout.second)) ;
							previous_vertex[eout.second] = vid ;
						}
					} // for each outgoing edge
				} // if ! visited
			} //  while 

			return 0 ;
		}

		int print_path(int toid) {
			if(toid < 0 || toid >= Dsource.size()) 
			{
				std::cerr << "Error print_path:  vertex id " << toid << " is out of range. " << std::endl ;
				return -1 ;
			}
		
			std::cout << "shortest path from vertex " << fid << " to " << toid << " has distance  " << Dsource[toid] << std::endl ;

			int pvid = previous_vertex[toid] ;
			std::cout << toid  ;
			while(pvid >=0) {
				std::cout << " <== " << pvid ; 
				pvid = previous_vertex[pvid] ;
			}
			std::cout << std::endl ;
			return 0; 
		}

	private: 

		int fid ;
		// distance from source 
		std::vector<int> Dsource ;

		// previous vertex on the optimum path to this node 
		std::vector<int> previous_vertex; 

		std::vector<gedge> v_edges ;

		// for non-queue based implementation 
		std::vector<bool> v_visited ;
		// 
		std::priority_queue<gedge, std::vector<gedge>,  std::greater<gedge> > Q ;

	}; 


} 



#endif 