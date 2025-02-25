// Lukas Larisch, 2014 - 2016
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 51 Franklin Street - Suite 500, Boston, MA 02110-1335, USA.
//
//
// Offers functionality to compute tree decompositions of graphs
// according to various heuristic and functions, that convert
// tree decompositions to elimination orderings and vice versa.
// Also the LEX-M algorithm is included in this header
//
//

#include <boost/tuple/tuple.hpp>
#include <map>
#include <boost/graph/adjacency_list.hpp>
#include "container.hpp"
#include "graph_traits.hpp"
#include "treedec.hpp"

// typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, treedec::bag_t> TD_tree_dec_t;
// TREEDEC_TREEDEC_BAG_TRAITS(TD_tree_dec_t, bag)
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, treedec::bag_t> TD_tree_dec_directed_t;
// TREEDEC_TREEDEC_BAG_TRAITS(TD_tree_dec_directed_t, bag);

#include "generic_base.hpp"
#include "graph.hpp"
#include "preprocessing.hpp"
#include "combinations.hpp"
#include "lower_bounds.hpp"
#include "elimination_orderings.hpp"
#include "nice_decomposition.hpp"
#include "applications/clique.hpp"
#include "applications/independent_set.hpp"
#include "applications/vertex_cover.hpp"
#include "applications/dominating_set.hpp"
#include "applications/coloring.hpp"
#include "misc.hpp"
#include "thorup.hpp"

#ifdef HAVE_GALA_GRAPH_H
#include <gala/boost.h>
#endif


#include "convenience.hpp"


typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS> TD_graph_t; //type 0
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> TD_graph_vec_t; //type 1
typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS> TD_graph_directed_t; //type 2
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS> TD_graph_directed_vec_t; //type 3


#include "python_tdlib.hpp"

template <typename G_t>
void make_tdlib_graph(G_t &G,
        std::vector<unsigned int> const&V,
        std::vector<unsigned int> const&E,
        bool directed=false){
    (void)directed; // not really sure, what this is supposed to do.
                    // G is directed. or it is not.
    unsigned int max = 0;
    for(unsigned int i = 0; i < V.size(); i++){
        max = (V[i]>max)? V[i] : max;
    }

    std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> idxMap(max+1);
    for(unsigned int i = 0; i < V.size(); i++){
        idxMap[i] = boost::add_vertex(G);
    }

    if(E.size() != 0){
        for(unsigned int j = 0; j < E.size()-1; j++){
            treedec::add_edge(idxMap[E[j]], idxMap[E[j+1]], G);
            j++;
        }
    }
}


template <typename T_t>
void make_tdlib_decomp(T_t &T, std::vector<std::vector<int> > &V, std::vector<unsigned int> &E){
    std::vector<typename T_t::vertex_descriptor> idxMap(V.size()+1);

    for(unsigned int i = 0; i < V.size(); i++){
        idxMap[i] = boost::add_vertex(T);
        std::set<unsigned int> bag;
        for(unsigned int j = 0; j < V[i].size(); j++){
            bag.insert(V[i][j]);
        }
        T[idxMap[i]].bag = bag;
    }

    if(E.size() != 0){
        for(unsigned int j = 0; j < E.size()-1; j++){
            boost::add_edge(idxMap[E[j]], idxMap[E[j+1]], T);
            j++;
        }
    }

}

template <typename G_t>
void make_python_graph(G_t &G, std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                       bool ignore_isolated_vertices=false)
{
    typename boost::graph_traits<G_t>::vertex_iterator vIt, vEnd;
    for(boost::tie(vIt, vEnd) = boost::vertices(G); vIt != vEnd; vIt++){
        if(ignore_isolated_vertices && boost::out_degree(*vIt, G) == 0){
            continue;
        }
        V_G.push_back(*vIt);
    }

    typename boost::graph_traits<G_t>::edge_iterator eIt, eEnd;
    for(boost::tie(eIt, eEnd) = boost::edges(G); eIt != eEnd; eIt++){
        E_G.push_back(boost::source(*eIt, G));
        E_G.push_back(boost::target(*eIt, G));
    }
}


template <typename T_t>
void make_python_decomp(T_t &T, std::vector<std::vector<int> > &V_T,
                        std::vector<unsigned int> &E_T)
{
    std::map<typename boost::graph_traits<T_t>::vertex_descriptor, unsigned int> vertex_map;
    typename boost::graph_traits<T_t>::vertex_iterator tIt, tEnd;
    unsigned int id = 0;

    for(boost::tie(tIt, tEnd) = boost::vertices(T); tIt != tEnd; tIt++){
        vertex_map.insert(std::pair<typename boost::graph_traits<T_t>::vertex_descriptor, unsigned int>(*tIt, id++));
        std::vector<int> bag;
        for(std::set<unsigned int>::iterator sIt = T[*tIt].bag.begin(); sIt != T[*tIt].bag.end(); sIt++){
            bag.push_back(*sIt);
        }
        V_T.push_back(bag);
    }

    typename boost::graph_traits<T_t>::edge_iterator eIt, eEnd;
    for(boost::tie(eIt, eEnd) = boost::edges(T); eIt != eEnd; eIt++){
        typename std::map<typename boost::graph_traits<T_t>::vertex_descriptor, unsigned int>::iterator v, w;
        v = vertex_map.find(boost::source(*eIt, T));
        w = vertex_map.find(boost::target(*eIt, T));
        E_T.push_back(v->second);
        E_T.push_back(w->second);
    }
}


/* PREPROCESSING */

int gc_preprocessing(std::vector<unsigned int> &V_G,
                     std::vector<unsigned int> &E_G,
                     std::vector<std::vector<int> > &bags, int lb, unsigned graphtype)
{
    typedef typename treedec::graph_traits<TD_graph_t>::treedec_type T;

    std::vector< boost::tuple<
        typename treedec::treedec_traits<T>::vd_type,
        typename treedec::treedec_traits<T>::bag_type
             > > td_bags;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::preprocessing(G, td_bags, lb);

        V_G.clear();
        E_G.clear();

        make_python_graph(G, V_G, E_G, true); //ignores isolated vertices
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::preprocessing(G, td_bags, lb);

        V_G.clear();
        E_G.clear();

        make_python_graph(G, V_G, E_G, true); //ignores isolated vertices
    }
    else{
        assert(false);
        return -66;
    }

    bags.resize(td_bags.size());
    for(unsigned int i = 0; i < td_bags.size(); i++){
        std::vector<int> bag;
        bag.push_back(td_bags[i].get<0>());
        for(typename treedec::treedec_traits<T>::bag_type::iterator sIt
                 = td_bags[i].get<1>().begin(); sIt != td_bags[i].get<1>().end(); sIt++)
        {
            bag.push_back(*sIt);
        }
        bags[i] = bag;
    }

    return lb;
}


int gc_PP_MD(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
             std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_MD(G, T, lb);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_MD(G, T, lb);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_PP_FI(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
             std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_FI(G, T, lb);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_FI(G, T, lb);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_PP_FI_TM(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
               std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_FI_TM(G, T, lb);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::PP_FI_TM(G, T, lb);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


/* LOWER BOUNDS */


int gc_deltaC_min_d(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_min_d(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_min_d(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_deltaC_max_d(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_max_d(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_max_d(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_deltaC_least_c(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_least_c(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::deltaC_least_c(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_LBN_deltaC(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBN_deltaC(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBN_deltaC(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_LBNC_deltaC(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBNC_deltaC(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBNC_deltaC(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_LBP_deltaC(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBP_deltaC(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBP_deltaC(G);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_LBPC_deltaC(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned graphtype){
    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBPC_deltaC(G);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::lb::LBPC_deltaC(G);
    }
    else{
        assert(false);
        return -66;
    }
}


/* EXACT TREE DECOMPOSITIONS */

int gc_exact_decomposition_ex17(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                  std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
  std::cerr << "gc_exact_decomposition_ex17 graphtype " << graphtype << "\n";
    TD_tree_dec_t T;
    // TD_graph_vec_t G;
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G); // BUG. why is there no graph yet?
    std::cerr << "ta G " << boost::num_vertices(G) << " " << boost::num_edges(G) << "\n";
#ifdef HAVE_GALA_GRAPH_H
    treedec::exact_decomposition_ex17(G, T, lb);
#else
    incomplete();
#endif
    std::cerr << "ta T " << boost::num_vertices(T) << " " << boost::num_edges(T) << "\n";
    // incomplete(); //really?

    assert(treedec::is_valid_treedecomposition(G, T));

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);
    std::cerr << "pythondecomp nvT" << boost::num_vertices(T) << "\n";
    std::cerr << "pythondecomp " << V_T.size() << " " << E_T.size() << "\n";

    return treedec::get_width(T);
}

int gc_exact_decomposition_cutset(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                  std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::exact_decomposition_cutset(G, T, lb);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::exact_decomposition_cutset(G, T, lb);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_exact_decomposition_cutset_decision(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                           std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int k, unsigned graphtype)
{
    TD_tree_dec_t T;

    bool rtn;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        rtn = treedec::exact_decomposition_cutset_decision(G, T, k);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        rtn = treedec::exact_decomposition_cutset_decision(G, T, k);
    }
    else{
        assert(false);
        return -66;
    }

    if(!rtn){
        return -1;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return 0;
}


int gc_exact_decomposition_dynamic(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                   std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, int lb, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::exact_decomposition_dynamic(G, T, lb);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::exact_decomposition_dynamic(G, T, lb);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


/* APPOXIMATIVE TREE DECOMPOSITIONS */

int gc_seperator_algorithm(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                           std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::separator_algorithm(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::separator_algorithm(G, T);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_minDegree_decomp(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                         std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::minDegree_decomp(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::minDegree_decomp(G, T);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_boost_minDegree_decomp(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                         std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
  (void)graphtype; // incomplete();?
    TD_graph_directed_vec_t G;
    make_tdlib_graph(G, V_G, E_G, true); //true = directed

    std::vector<int> O;

#ifndef NDEBUG
    unsigned w1 =
#endif 
    treedec::impl::boost_minDegree_ordering(G, O);

    TD_graph_t H;
    make_tdlib_graph(H, V_G, E_G);
    TD_tree_dec_t T;
    treedec::draft::vec_ordering_to_tree(H, O, T, (std::vector<int>*)nullptr);

#ifndef NDEBUG
    unsigned w2 =
#endif
    treedec::get_bagsize(T);

#ifndef NDEBUG
    // assert(w1 == w2); // why not?
    (void)w1; (void)w2;
#endif

    treedec::make_small(T);

    make_python_decomp(T, V_T, E_T);
    return treedec::get_width(T);
}


int gc_fillIn_decomp(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                      std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0 /* bug, use enum! */ ){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::fillIn_decomp(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::fillIn_decomp(G, T);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}



typedef bool cfg_alive_t;
typedef bool cfg_dying_t;

struct cfg_node
{
  // iCode *ic; // bnot relevant
  // operand_map_t operands;
  cfg_alive_t alive;
  cfg_dying_t dying;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, cfg_node> cfg_t;



int gc_Thorup(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                      std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        //TD_graph_t G;
        cfg_t G;

        make_tdlib_graph(G, V_G, E_G, true); //directed

        typedef treedec::he::thorup<cfg_t> thorup;
        //typedef treedec::he::thorup<TD_graph_t> thorup;
	thorup Tp(G);
	Tp.do_it();
	Tp.get_tree_decomposition(T);
    }
    else if(graphtype == 1){
        //TD_graph_vec_t G;
        cfg_t G;

        make_tdlib_graph(G, V_G, E_G, true); //directed

        typedef treedec::he::thorup<cfg_t> thorup;
        //typedef treedec::he::thorup<TD_graph_t> thorup;
	thorup Tp(G);
	Tp.do_it();
	Tp.get_tree_decomposition(T);
    }
    else{
        assert(false);
        return -66;
    }

    treedec::make_small(T);
    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


void gc_minDegree_ordering(std::vector<unsigned int> &V, std::vector<unsigned int> &E,
                           std::vector<unsigned int> &elim_ordering, unsigned graphtype)
{
    std::vector<TD_graph_t::vertex_descriptor> elim_ordering_;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V, E);

        treedec::minDegree_ordering(G, elim_ordering_);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V, E);

        treedec::minDegree_ordering(G, elim_ordering_);
    }
    else{
        assert(false);
    }

    elim_ordering.resize(V.size());
    for(unsigned int i = 0; i < elim_ordering_.size(); i++){
        elim_ordering[i] = elim_ordering_[i];
    }
}


void gc_fillIn_ordering(std::vector<unsigned int> &V, std::vector<unsigned int> &E, std::vector<unsigned int> &elim_ordering, unsigned graphtype){
    std::vector<TD_graph_t::vertex_descriptor> elim_ordering_;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V, E);

        treedec::fillIn_ordering(G, elim_ordering_);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V, E);

        treedec::fillIn_ordering(G, elim_ordering_);
    }
    else{
        assert(false);
    }

    elim_ordering.resize(V.size());
    for(unsigned int i = 0; i < elim_ordering_.size(); i++){
        elim_ordering[i] = elim_ordering_[i];
    }
}


/* POSTPROCESSING */

int gc_MSVS(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
            std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::MSVS(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::MSVS(G, T);
    }
    else{
        assert(false);
        return -66;
    }

    V_T.clear();
    E_T.clear();

    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}

void gc_minimalChordal(std::vector<unsigned int> &V, std::vector<unsigned int> &E,
                       std::vector<unsigned int> &old_elimination_ordering,
                       std::vector<unsigned int> &new_elimination_ordering, unsigned graphtype)
{
    typename std::vector<boost::graph_traits<TD_graph_t>::vertex_descriptor>
                   old_elimination_ordering_tmp(old_elimination_ordering.size());

    for(unsigned int i = 0; i < old_elimination_ordering.size(); i++){
        old_elimination_ordering_tmp[i] = old_elimination_ordering[i];
    }

    typename std::vector<boost::graph_traits<TD_graph_t>::vertex_descriptor>
                                                     new_elimination_ordering_tmp;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V, E);

        treedec::minimalChordal(G, old_elimination_ordering_tmp, new_elimination_ordering_tmp);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V, E);

        treedec::minimalChordal(G, old_elimination_ordering_tmp, new_elimination_ordering_tmp);
    }
    else{
        assert(false);
    }

    new_elimination_ordering.resize(new_elimination_ordering_tmp.size());
    for(unsigned int i = 0; i < new_elimination_ordering_tmp.size(); i++){
        new_elimination_ordering[i] = (unsigned int) new_elimination_ordering_tmp[i];
    }
}


/* APPLICATIONS */

unsigned gc_max_clique_with_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                          std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                                          std::vector<unsigned int> &C, bool certificate, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);
    treedec::make_small(T);

    std::set<unsigned int> result;

    unsigned size = 0;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::max_clique_with_treedecomposition(G, T, result, certificate);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::max_clique_with_treedecomposition(G, T, result, certificate);
    }
    else{
        assert(false);
    }

    C.resize(result.size());
    unsigned int i = 0;
    for(std::set<unsigned int>::iterator sIt = result.begin(); sIt != result.end(); sIt++){
        C[i++] = *sIt;
    }

    return size ;
}

unsigned gc_max_independent_set_with_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                                   std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                                                   std::vector<unsigned int> &IS, bool certificate, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    TD_tree_dec_directed_t T_;
    treedec::make_rooted(T, T_);

    treedec::nice::I_nicify<TD_tree_dec_directed_t> N(T_, false);
    N.do_it();

    std::set<unsigned int> result;

    unsigned size = 0;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::max_independent_set_with_treedecomposition(G, T_, result, certificate);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::max_independent_set_with_treedecomposition(G, T_, result, certificate);
    }
    else{
        assert(false);
    }

    IS.resize(result.size());
    unsigned int i = 0;
    for(std::set<unsigned int>::iterator sIt = result.begin(); sIt != result.end(); sIt++){
        IS[i++] = *sIt;
    }

    return size;
}


unsigned gc_min_vertex_cover_with_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                                std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                                                std::vector<unsigned int> &VC, bool certificate, bool cache_traversal, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    TD_tree_dec_directed_t T_;
    treedec::make_rooted(T, T_);

    treedec::nice::nicify(T_);

    std::set<unsigned int> result;

    unsigned size = 0;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_vertex_cover_with_treedecomposition(G, T_, result, certificate, cache_traversal);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_vertex_cover_with_treedecomposition(G, T_, result, certificate, cache_traversal);
    }
    else{
        assert(false);
    }

    VC.resize(result.size());
    unsigned int i = 0;
    for(std::set<unsigned int>::iterator sIt = result.begin(); sIt != result.end(); sIt++){
        VC[i++] = *sIt;
    }

    return size;
}


unsigned gc_min_dominating_set_with_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                                  std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                                                  std::vector<unsigned int> &DS, bool certificate, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    TD_tree_dec_directed_t T_;
    treedec::make_rooted(T, T_);

    treedec::nice::nicify(T_);

    std::set<unsigned int> result;

    unsigned size = 0;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_dominating_set_with_treedecomposition(G, T_, result, certificate);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_dominating_set_with_treedecomposition(G, T_, result, certificate);
    }
    else{
        assert(false);
    }

    DS.resize(result.size());
    unsigned int i = 0;
    for(std::set<unsigned int>::iterator sIt = result.begin(); sIt != result.end(); sIt++){
        DS[i++] = *sIt;
    }

    return size;
}


unsigned gc_min_coloring_with_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                        std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                                        std::vector<std::vector<int> > &C, bool certificate, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    TD_tree_dec_directed_t T_;
    treedec::make_rooted(T, T_);

    treedec::nice::nicify(T_);

    std::vector<std::set<unsigned int> > result;

    unsigned size = 0;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_coloring_with_treedecomposition(G, T_, result, certificate);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        size = treedec::app::min_coloring_with_treedecomposition(G, T_, result, certificate);
    }
    else{
        assert(false);
    }

    C.resize(result.size());
    for(unsigned int i = 0; i < result.size(); i++){
        for(std::set<unsigned int>::iterator sIt = result[i].begin(); sIt != result[i].end(); sIt++){
            C[i].push_back(*sIt);
        }
    }

    return size;
}


/* MISC */

int gc_ordering_to_treedec(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                           std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T,
                           std::vector<unsigned int> &elim_ordering, unsigned graphtype)
{
    TD_tree_dec_t T;

    std::vector<TD_graph_t::vertex_descriptor> elim_ordering_(V_G.size());
    for(unsigned int i = 0; i < elim_ordering.size(); i++){
        elim_ordering_[i] = elim_ordering[i];
    }

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::ordering_to_treedec(G, elim_ordering_, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::ordering_to_treedec(G, elim_ordering_, T);
    }
    else{
        assert(false);
        return -66;
    }

    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


void gc_treedec_to_ordering(std::vector<std::vector<int> > &V, std::vector<unsigned int> &E, std::vector<unsigned int> &elim_ordering){
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V, E);

    std::vector<boost::graph_traits<TD_graph_t>::vertex_descriptor> elim_ordering_;
    treedec::treedec_to_ordering(T, elim_ordering_);

    elim_ordering.resize(elim_ordering_.size());
    for(unsigned int i = 0; i < elim_ordering_.size(); i++){
        elim_ordering[i] = (unsigned int) elim_ordering_[i];
    }
}


int gc_trivial_decomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, std::vector<std::vector<int> > &V_T,
                             std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::trivial_decomposition(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        treedec::trivial_decomposition(G, T);
    }
    else{
        assert(false);
        return -66;
    }

    make_python_decomp(T, V_T, E_T);

    return treedec::get_width(T);
}


int gc_validate_treedecomposition(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                  std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned graphtype)
{
    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    if(graphtype == 0){
        TD_graph_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::validate_treedecomposition(G, T);
    }
    else if(graphtype == 1){
        TD_graph_vec_t G;
        make_tdlib_graph(G, V_G, E_G);

        return treedec::validate_treedecomposition(G, T);
    }
    else{
        assert(false);
        return -66;
    }
}


int gc_get_width(std::vector<std::vector<int> > &V_T){
    int width = 0;
    for(unsigned int i = 0; i< V_T.size(); i++){
        width = ((int)V_T[i].size() > width)? (int)V_T[i].size() : width;
    }
    return width-1;
}


/* Generic elimination search */

void gc_generic_elimination_search1(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned /*graphtype*/, unsigned max_nodes, unsigned max_orderings){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    treedec::generic_elimination_search_CFG1(G, max_nodes, max_orderings);
}

void gc_generic_elimination_search2(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned /*graphtype*/, unsigned max_nodes, unsigned max_orderings){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    treedec::generic_elimination_search_CFG2(G, max_nodes, max_orderings);
}

void gc_generic_elimination_search3(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned /*graphtype*/, unsigned max_nodes, unsigned max_orderings){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    treedec::generic_elimination_search_CFG3(G, max_nodes, max_orderings);
}

void gc_generic_elimination_search_p17(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned /*graphtype*/, unsigned max_nodes, unsigned max_orderings){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    treedec::generic_elimination_search_p17(G, max_nodes, max_orderings);
}

void gc_generic_elimination_search_p17_jumper(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G, unsigned /*graphtype*/, unsigned max_nodes, unsigned max_orderings){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    treedec::generic_elimination_search_p17_jumper(G, max_nodes, max_orderings);
}


/* weight stuff */



unsigned gc_weight_stats(std::vector<unsigned int> &V_G, std::vector<unsigned int> &E_G,
                                  std::vector<std::vector<int> > &V_T, std::vector<unsigned int> &E_T, unsigned /* graphtype */, bool verbose){
    TD_graph_t G;
    make_tdlib_graph(G, V_G, E_G);

    TD_tree_dec_t T;
    make_tdlib_decomp(T, V_T, E_T);

    TD_tree_dec_directed_t N, M;

    treedec::make_rooted(T, M);

    treedec::nice::nicify(M);

    std::stack<boost::graph_traits<TD_tree_dec_directed_t>::vertex_descriptor> S;

    treedec::nice::min_weight_traversal_caller(M, S);

    //dump_td(M, "weight_stuff.dot");

    return treedec::nice::weight_try_roots(T, N, verbose);
}

// vim:ts=8:sw=4:et
