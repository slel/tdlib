// Felix Salfelder, 2015 - 2016
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

/*
 * Graph routines related to tree decompositions.
 *
 * Implemented for generic BGL graphs. Possibly (much) faster for particular
 * graphs.
 *
 * template<typename nIter_t, typename G_t>
 *    void make_clique(nIter_t nIter, G_t &G)
 *
 * template<typename B_t, typename nIter_t, typename G_t>
 *    void copy_neighbourhood(B_t &B, nIter_t nIter, G_t &G)
 *
 * template<class G>
 * void detach_neighborhood(typename boost::graph_traits<G>::vertex_descriptor& c,
 *    G& g, typename outedge_set<G>::type& bag)
 */

#ifndef TREEDECGRAPH_HPP
#define TREEDECGRAPH_HPP

#include <boost/graph/adjacency_list.hpp>

#include "error.hpp"
#include "graph_traits.hpp"
#include "treedec_traits.hpp"
#include "platform.hpp"
#include "random_generators.hpp"
#include "trace.hpp"
#include "error.hpp"
#include "bits/bool.hpp"

// not yet
// #include <gala/boost.h>
// #include "graph_gala.hpp"

#include "degree.hpp"

//#include "generic_elimination_search_overlay.hpp"
// #include "induced_subgraph.hpp"
#include "graph_impl.hpp"

namespace treedec{

#define vertex_iterator_G typename boost::graph_traits<G>::vertex_iterator
#define vertex_descriptor_G typename boost::graph_traits<G>::vertex_descriptor
#define adjacency_iterator_G typename boost::graph_traits<G>::adjacency_iterator


// obsolete forward? better don't use
#if 0
template<typename G_t>
inline typename boost::graph_traits<G_t>::vertices_size_type
   get_pos(typename boost::graph_traits<G_t>::vertex_descriptor v, G_t const& G)
{ untested();
    return boost::get(boost::vertex_index, G, v);
}
#endif

//Vertex v will remain as isolated node.
//Calls cb on neighbors if degree drops by one,
//before dg will drop.
// TODO: pass edge or edge iterator.
template<typename G>
void contract_edge(vertex_descriptor_G v,
                   vertex_descriptor_G target,
                   G &g,
                   vertex_callback<vertex_descriptor_G>* cb=NULL)
{
    adjacency_iterator_G I, E;
    for(boost::tie(I, E)=boost::adjacent_vertices(v, g); I!=E; ++I){
        assert(boost::edge(v, *I, g).second);
        if(*I != target){
            bool added=treedec::add_edge(target, *I, g).second;
            if(added){
                //rebasing edge from I-v to I-target.
            }
            else if(cb){ untested();
                //did not add, degree will drop by one.
                (*cb)(*I);
            }
        }
    }

    boost::clear_vertex(v, g);
    assert(!boost::edge(v, target, g).second);
}


// turn vertex range into clique.
// call cb on newly created edges and incident vertices.
// (no longer?!) returns the number of newly created edges.
template<typename B, typename E, typename G_t>
void make_clique(B nIt1, E nEnd, G_t &G, typename treedec::graph_callback<G_t>* cb=NULL)
{
    return impl::make_clique(nIt1, nEnd, G, cb);
}

template<typename nIter_t, typename G_t>
void make_clique(nIter_t nIter, G_t &G, treedec::graph_callback<G_t>* cb=NULL)
{
    typename boost::graph_traits<G_t>::adjacency_iterator nIt1, nEnd;
    boost::tie(nIt1, nEnd) = nIter;
    make_clique(nIt1, nEnd, G, cb);
}

// insert the neighbors of v in G into B
template<typename B_t, typename V_t, typename G_t>
void insert_neighbours(B_t &B, V_t v, G_t const &G)
{
    typename boost::graph_traits<G_t>::adjacency_iterator nIt, nEnd;
    boost::tie(nIt, nEnd) = boost::adjacent_vertices(v, G);
    for(; nIt!=nEnd; ++nIt){
        B.insert(*nIt);
    }
}

template<typename B_t, typename V_t, typename G_t>
void insert_neighbours(B_t &B, V_t v, V_t w, G_t const &G)
{
    insert_neighbours(B, v, G);
    insert_neighbours(B, w, G);
}

template<typename B_t, typename V_t, typename G_t>
void insert_neighbours(B_t &B, V_t v, V_t w, V_t x, G_t const &G)
{ untested();
    insert_neighbours(B, v, w, G);
    insert_neighbours(B, x, G);
}

// insert the neighbors of v in G into B
// B starts empty.
// equivalent to B = outedge_set(v, G) where applicable
template<typename B_t, typename V_t, typename G_t>
void assign_neighbours(B_t &B, V_t v, G_t const &G)
{ untested();
    typename boost::graph_traits<G_t>::adjacency_iterator nIt, nEnd;
    boost::tie(nIt, nEnd) = boost::adjacent_vertices(v, G);
    for(; nIt!=nEnd; ++nIt){ untested();
        B.insert(B.end(), *nIt);
    }
}

// insert neighbors of both vertices, after clearing B
template<typename B_t, typename V_t, typename G_t>
void assign_neighbours(B_t &B, V_t v, V_t w, V_t x, G_t const &G)
{
    B.clear();
    insert_neighbours(B, v, w, G);
    insert_neighbours(B, x, G);
}

// find a minimum degree vertex using linear search.
template <typename G_t>
inline typename boost::graph_traits<G_t>::vertex_descriptor
   get_min_degree_vertex(const G_t &G, bool ignore_isolated_vertices=false)
{
    unsigned int min_degree = UINT_MAX;

    typename boost::graph_traits<G_t>::vertex_iterator vIt, vEnd;
    boost::tie(vIt, vEnd) = boost::vertices(G);
    typename boost::graph_traits<G_t>::vertex_descriptor min_vertex = *vIt++;
    for(; vIt != vEnd; vIt++){
        unsigned int degree = boost::out_degree(*vIt, G);
        if(degree <= min_degree){
            if(ignore_isolated_vertices && degree == 0){ continue; }
            min_degree = degree;
            min_vertex = *vIt;
        }
    }

    return min_vertex;
}

template <typename G_t, class M>
inline typename boost::graph_traits<G_t>::vertex_descriptor
   get_least_common_vertex(const typename boost::graph_traits<G_t>::vertex_descriptor min_vertex,
           M& marker,
           const G_t &G);

//Copy vertices of G into degree_sequence, ordered by degree, starting with lowest.
template <typename G_t>
inline void make_degree_sequence(const G_t &G,
          std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> &degree_sequence)
{ untested();
    unsigned int max_degree = 0;
    typename boost::graph_traits<G_t>::vertex_iterator vIt, vEnd;
    for(boost::tie(vIt, vEnd) = boost::vertices(G); vIt != vEnd; vIt++){ untested();
        unsigned int degree = boost::out_degree(*vIt, G);
        max_degree = (degree>max_degree)? degree : max_degree;
    }

    std::vector<std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> > buckets(max_degree+1);
    for(boost::tie(vIt, vEnd) = boost::vertices(G); vIt != vEnd; vIt++){ untested();
        unsigned int degree = boost::out_degree(*vIt, G);
        if(degree > 0){ untested();
            buckets[degree].push_back(*vIt);
        }
    }
    for(unsigned int i = 1; i <= max_degree; i++){ untested();
        for(unsigned int j = 0; j < buckets[i].size(); j++){ untested();
            degree_sequence.push_back(buckets[i][j]);
        }
    }
}

// obsolete?
template<class G>
struct outedge_set;

template <typename G_t>
std::pair<typename boost::graph_traits<typename graph_traits<G_t>::directed_overlay>::vertex_descriptor,
          typename boost::graph_traits<typename graph_traits<G_t>::directed_overlay>::vertex_descriptor>
    make_digraph_with_source_and_sink(G_t const &G, std::vector<BOOL> const &disabled,
                 unsigned num_dis,
                 typename graph_traits<G_t>::directed_overlay& dg,
                 std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> &idxMap,
                 typename std::set<typename boost::graph_traits<G_t>::vertex_descriptor> const &X,
                 typename std::set<typename boost::graph_traits<G_t>::vertex_descriptor> const &Y);


template<class G_t>
struct deg_chooser {
    typedef typename misc::DEGS<G_t> type;
    typedef type degs_type; // transition? don't use.
};

// put neighbors of c into bag
// isolate c in g
// return bag
// optionally: pass pointer to bag for storage.

//TODO: not here.
template<class G_t, class B_t>
inline void detach_neighborhood(
        typename boost::graph_traits<G_t>::vertex_descriptor c,
        G_t& g, typename std::set<B_t> &bag)
{
    assert(boost::is_undirected(g));

    typename boost::graph_traits<G_t>::adjacency_iterator nIt1, nIt2, nEnd;
    // inefficient.

    for(boost::tie(nIt1, nEnd) = boost::adjacent_vertices(c, g); nIt1 != nEnd; nIt1++) {
        bag.insert(*nIt1);
    }
    boost::clear_vertex(c, g);
}

//TODO: not here.
template<class G_t, class B_t>
inline void detach_neighborhood(
        typename boost::graph_traits<G_t>::vertex_descriptor c,
        G_t& g, typename std::vector<B_t> &bag)
{
    assert(boost::is_undirected(g));

    typename boost::graph_traits<G_t>::adjacency_iterator nIt1, nIt2, nEnd;
    // inefficient.

    unsigned i = 0;
    for(boost::tie(nIt1, nEnd) = boost::adjacent_vertices(c, g); nIt1 != nEnd; nIt1++) {
        assert(i<bag.size());
        bag[i++]=*nIt1;
    }
    boost::clear_vertex(c, g);
}

// collect neighbors of c into bag
// remove c from graph (i.e. isolate).
// turn subgraph induced by bag into clique.
//
// optionally:
// - call back on each newly created edge, and on each vertex incident to such
//   edge.
// - provide memory through bag pointer.
//
template<class G, class B>
void make_clique_and_detach(
        typename boost::graph_traits<G>::vertex_descriptor c,
        G& g, B& bag,
        treedec::graph_callback<G>* cb=NULL)
{
    detach_neighborhood(c, g, bag);
    make_clique(bag.begin(), bag.end(), g, cb);
}

namespace detail{ //

    // iterate over edges adjacent to both v and s
    // implementation: iterate outedges(v), skip non-outedges(s).
    template<class G>
    class shared_adj_iter : public boost::graph_traits<G>::adjacency_iterator{ //
    public:
        typedef typename boost::graph_traits<G>::adjacency_iterator adjacency_iterator;
        typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;
        typedef typename boost::graph_traits<G>::adjacency_iterator baseclass;
    public: // construct
        shared_adj_iter(adjacency_iterator v, adjacency_iterator ve,
                        vertex_descriptor s, G const& g)
            : adjacency_iterator(v), _ve(ve),
              _s(s), _g(g)
        {
            skip();
        }
        shared_adj_iter(vertex_descriptor v,
                        vertex_descriptor s, G const& g)
            : adjacency_iterator(boost::adjacent_vertices(v, g).first),
              _ve(boost::adjacent_vertices(v, g).second),
              _s(s), _g(g)
        {untested();
            skip();
        }
        shared_adj_iter(const shared_adj_iter& p)
            : adjacency_iterator(p), _ve(p._ve), _s(p._s), _g(p._g)
        {
        }
    public: //ops
        shared_adj_iter& operator++(){
            assert(_ve!=adjacency_iterator(*this));
            assert(adjacency_iterator(*this)!=_ve);
            adjacency_iterator::operator++();
            skip();

            return *this;
        }
    private:
        void skip()
        {
            while(true){
                if(baseclass(*this)==_ve){
                    return;
                }else if(!boost::edge(**this, _s, _g).second){
                    adjacency_iterator::operator++();
                }else{
                    return;
                }
            }
        }
        adjacency_iterator _ve;
        vertex_descriptor _s;
        G const& _g;
    };

}// detail

// inefficient.
// possibly obsolete.
template<class G>
std::pair<detail::shared_adj_iter<G>, detail::shared_adj_iter<G> >
inline common_out_edges(typename boost::graph_traits<G>::vertex_descriptor v,
        typename boost::graph_traits<G>::vertex_descriptor w,
        const G& g)
{
    typedef typename detail::shared_adj_iter<G> Iter;

    auto p=boost::adjacent_vertices(v, g);
    return std::make_pair(Iter(p.first, p.second, w, g),
                          Iter(p.second, p.second, w, g));
}

// check if two vertices have the same neighborhood.
// return false if not.
template<class G_t>
bool check_twins(typename boost::graph_traits<G_t>::vertex_descriptor v,
                 typename boost::graph_traits<G_t>::vertex_descriptor w,
                 const G_t& G)
{ untested();
    typename graph_traits<G_t>::outedge_set_type N1, N2;
    assign_neighbours(N1, v, G);
    assign_neighbours(N2, w, G);

    return(N1==N2);
}

// create ig.
// edge(v, w, ig) == edge(ordering[v], ordering[w], g)
// TODO: inverse map?
// TODO: partial/induced graph?
template<class G>
void immutable_clone(G const &g, typename graph_traits<G>::immutable_type& ig,
   std::vector<typename boost::graph_traits<G>::vertex_descriptor>* ordering=NULL)
{ untested();
    typedef typename graph_traits<G>::immutable_type immutable_type;
//    typedef typename graph_traits<G>::vertex_iterator vertex_iterator;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;
    typedef typename boost::graph_traits<G>::edge_iterator edge_iterator;

    if(!ordering){ incomplete();
    }
    assert(ordering->size()==boost::num_vertices(g)); // for now.


    if(boost::num_vertices(g) != boost::num_vertices(ig)){ untested();
        // drop a new one... (for now?)
        // inefficient.
        ig = MOVE(immutable_type(boost::num_vertices(g)));
    }else if(boost::num_edges(ig)){ untested();
        for(unsigned i=0; i<boost::num_vertices(g); ++i){ untested();
            boost::clear_vertex(i, ig);
        }
    }

    std::vector<unsigned> inverse_ordering(boost::num_vertices(g));
    for(unsigned o=0; o<boost::num_vertices(g); ++o){ untested();
        assert((*ordering)[o] < inverse_ordering.size());
        inverse_ordering[(*ordering)[o]] = o;
    }

    for(unsigned o=0; o<boost::num_vertices(g); ++o){ untested();
        assert((*ordering)[inverse_ordering[o]] == o);
    }

    edge_iterator e, eend;
    boost::tie(e,eend) = boost::edges(g);
    for(;e!=eend; ++e){ untested();
        vertex_descriptor s = boost::source(*e, g);
        vertex_descriptor t = boost::target(*e, g);
        assert(inverse_ordering[s] < boost::num_vertices(ig));
        assert(inverse_ordering[t] < boost::num_vertices(ig));
        assert((s==t) == (inverse_ordering[s]==inverse_ordering[t]));
        boost::add_edge(inverse_ordering[s], inverse_ordering[t], ig);
    }
}

// check if {*vd1, *vd2} is a subset of a bag adjacent to t.
template<class VD_t, class T_t>
class is_in_neighbour_bd{
public:
    is_in_neighbour_bd(T_t const& T,
        typename boost::graph_traits<T_t>::vertex_descriptor t)
       : _T(T), _t(t)
    {
    }
public:
    bool operator() (VD_t vd1, VD_t vd2)
    {
        assert(vd1!=vd2);
        if(vd1<vd2){
        }

        typedef typename boost::graph_traits<T_t>::adjacency_iterator bag_iterator;
        bag_iterator nIt, nEnd;
        boost::tie(nIt, nEnd) = boost::adjacent_vertices(_t, _T);
        for(; nIt!=nEnd; ++nIt){
            //auto const& ibag=boost::get(bag_t(), _T, *nIt);
            auto const& ibag=bag(*nIt, _T); // BUG

            // BUG, does not work on vectors.
            if(ibag.find(vd1)==ibag.end()){
            }else if(ibag.find(vd2)==ibag.end()){
            }else{
                return true;
            }
        }
        // a = vd1;
        // b = vd2;
        return false;
    }
private: //data
    T_t const &_T;
    typename boost::graph_traits<T_t>::vertex_descriptor _t;
public: // HACK
    long unsigned a{0l};
    long unsigned b{0l};
};

// clone subgraph induced by bag into ig.
// store map V(H) -> X \subset V(G) in vdMap
// add more edges for MSVS, TODO: implement properly (how?)
template<class G_t, class T_t, class IG_t, class M_t>
inline void induced_subgraph_with_extra_edges
(G_t const &G, IG_t& ig, // typename graph_traits<G>::immutable_type& ig,
       T_t const& T, typename boost::graph_traits<T_t>::vertex_descriptor bd,
    //   URGHS. no default types without c++11.
     M_t* vdMap /*=NULL*/);


// insert reverse edges.
// dont know how to express "orientation" yet
template<class G>
void make_symmetric(G&g, bool force_oriented=false)
{ untested();
    // assert(g.is_directed);
    auto EE=boost::edges(g);
    if ( /*g.oriented ||*/ force_oriented) { untested();
        for(;EE.first!=EE.second; ++EE.first){ untested();
            auto s=boost::source(*EE.first, g);
            auto t=boost::target(*EE.first, g);
            boost::add_edge(t,s,g);
        }
    }else{ untested();
        for(;EE.first!=EE.second; ++EE.first){ untested();
            auto s=boost::source(*EE.first, g);
            auto t=boost::target(*EE.first, g);
            if(!boost::edge(s,t,g).second){ untested();
                unreachable(); //?
                boost::add_edge(s,t,g);
            }else if(!boost::edge(t,s,g).second){ untested();
                boost::add_edge(t,s,g);
            }
        }
    }
}

namespace detail{

// fallback, undirected graphs
template<class G, class X=void>
struct edge_helper{
#if 0
    typedef typename boost::graph_traits<G>::edges_size_type size_type;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;

    static size_type num(G const& g){ untested();
        return boost::num_edges(g);
    }
    static std::pair<typename boost::graph_traits<G>::edge_descriptor, bool>
        add(vertex_descriptor x, vertex_descriptor y, G& g)
    { untested();
        BOOST_STATIC_ASSERT(std::is_convertible<
                             typename boost::graph_traits<G>::directed_category*,
                             boost::undirected_tag* >::value);

	return boost::add_edge(x, y, g);
    }
#endif
};

// bidirectional and directed
template<class G>
struct edge_helper<G, typename std::enable_if< std::is_convertible<
                             typename boost::graph_traits<G>::traversal_category*,
                             boost::bidirectional_graph_tag* >::value
                             && std::is_convertible<
                             typename boost::graph_traits<G>::directed_category*,
                             boost::directed_tag* >::value ,
                       void>::type >
{ //
    typedef typename boost::graph_traits<G>::edges_size_type size_type;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;

    static size_type num(G const& g){
        return boost::num_edges(g);
    }
    static std::pair<typename boost::graph_traits<G>::edge_descriptor, bool>
    add(vertex_descriptor x, vertex_descriptor y, G& g){ untested();

        BOOST_STATIC_ASSERT(
                std::is_convertible<typename boost::graph_traits<G>::traversal_category*,
                             boost::bidirectional_graph_tag* >::value);
        BOOST_STATIC_ASSERT(
                std::is_convertible<typename boost::graph_traits<G>::directed_category*,
                             boost::directed_tag* >::value);

        trace2("add dir bid", y ,x);
	return boost::add_edge(x, y, g);
    }
}; // edge_helper, bidirectional and directed

// bidirectional and undirected
template<class G>
struct edge_helper<G, typename std::enable_if< std::is_convertible<
                             typename boost::graph_traits<G>::traversal_category*,
                             boost::bidirectional_graph_tag* >::value
                             && std::is_convertible<
                             typename boost::graph_traits<G>::directed_category*,
                             boost::undirected_tag* >::value ,
                       void>::type >
{ //
    typedef typename boost::graph_traits<G>::edges_size_type size_type;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;

    static size_type num(G const& g){
        return boost::num_edges(g);
    }
    static std::pair<typename boost::graph_traits<G>::edge_descriptor, bool>
        add(vertex_descriptor x, vertex_descriptor y, G& g){
        trace2("add !dir bid", y ,x);

	return boost::add_edge(x, y, g);
    }
}; // edge_helper, bidirectional and undirected

// edge_helper, directed but not bidirectional.
template<class G>
struct edge_helper<G, typename std::enable_if<
                !std::is_convertible<
                    typename boost::graph_traits<G>::traversal_category*,
                    boost::bidirectional_graph_tag* >::value
                    && std::is_convertible<
                       typename boost::graph_traits<G>::directed_category*,
                       boost::directed_tag* >::value,
                    void>::type >
{ //
    typedef typename boost::graph_traits<G>::edges_size_type size_type;
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_descriptor;

    static size_type num(G const& g){
        assert(1 ^ boost::num_edges(g)) ;
        return boost::num_edges(g)/2;
    }
    static std::pair<typename boost::graph_traits<G>::edge_descriptor, bool>
        add(vertex_descriptor x, vertex_descriptor y, G& g)
    {
        trace2("add dir !bid", y ,x);
        BOOST_STATIC_ASSERT(
                !std::is_convertible<typename boost::graph_traits<G>::traversal_category*,
                             boost::bidirectional_graph_tag* >::value);
        BOOST_STATIC_ASSERT(
                std::is_convertible<typename boost::graph_traits<G>::directed_category*,
                             boost::directed_tag* >::value);

	boost::add_edge(y, x, g);
	return boost::add_edge(x, y, g);
    }
}; // edge_helper, directed but not bidirectional.

}

template<class G>
inline typename boost::graph_traits<G>::edges_size_type
  num_edges(G const& g)
{
    return detail::edge_helper<G>::num(g);
}

template<class G>
inline std::pair<typename boost::graph_traits<G>::edge_descriptor, bool>
add_edge(typename boost::graph_traits<G>::vertex_descriptor x,
		   typename boost::graph_traits<G>::vertex_descriptor y, G& g)
{
    return detail::edge_helper<G>::add(x, y, g);
}

template<class S, class G>
void open_neighbourhood(S& s, G const& g)
{
    graph_helper<G>::open_neighbourhood(s, g);
}

template<class S, class G>
void close_neighbourhood(S& s, G const& g)
{
    graph_helper<G>::close_neighbourhood(s, g);
}

template<class S, class G>
void saturate(S& s, G const& g)
{
    graph_helper<G>::saturate(s, g);
}





/* checks for preconditions prior to running an algorithm */
template <typename G_t>
bool is_undirected_type(G_t &G){
    (void)G;

    G_t H;

    auto v = boost::add_vertex(H);
    auto w = boost::add_vertex(H);
    boost::add_edge(v, w, H);

    return boost::edge(w, v, H).second;
}

template <typename G_t>
//works with vertex container=vec
bool is_edge_set_type(G_t &G){
    (void)G;

    G_t H;
    auto u = boost::add_vertex(H);
    auto v = boost::add_vertex(H);
    auto w = boost::add_vertex(H);

    boost::add_edge(u, w, H);
    boost::add_edge(u, v, H);

    typename boost::graph_traits<G_t>::adjacency_iterator nIt, nEnd;
    boost::tie(nIt, nEnd) = boost::adjacent_vertices(u, H);

    auto x = *nIt;
    nIt++;
    auto y = *nIt;

    return x < y;
}

//checks if G has no self-loops
template <typename G_t>
bool no_loops(G_t const &G){
    auto p=boost::vertices(G);
    for(; p.first!=p.second; ++p.first){
        if(boost::edge(*p.first, *p.first, G).second){
            return false;
        }
    }
    return true;
}

//checks if G has no duplicated edges
//TODO: dont use sets but markers (see one neighbour more than once -> return false)
template <typename G_t>
bool no_duplicate_edges(G_t const &G){
    auto p=boost::vertices(G);
    for(; p.first!=p.second; ++p.first){ 
        typename std::set<typename boost::graph_traits<G_t>::vertex_descriptor> S;
        typename boost::graph_traits<G_t>::adjacency_iterator nIt, nEnd;
        for(boost::tie(nIt, nEnd) = boost::adjacent_vertices(*p.first, G); nIt != nEnd; nIt++){
            if(S.find(*nIt) != S.end()){
                return false;
            }
            S.insert(*nIt);
        }
    }
    return true;
}

//checks if G is symmetric (v -> w => w -> v)
template <typename G_t>
bool is_symmetric(G_t const &G){
    auto p=boost::vertices(G);
    for(; p.first!=p.second; ++p.first){
        auto q=boost::adjacent_vertices(*p.first, G);
        for(; q.first!=q.second; ++q.first){
            if(!boost::edge(*q.first, *p.first, G).second){
                return false;
            }
        }
    }
    return true;
}

//this means the edge set, not the boost-type
template <typename G_t>
bool is_undirected(G_t const &G){
    return is_symmetric(G);
}

//checks if G is antisymmetric (v -> w => !(w -> v))
template <typename G_t>
bool is_antisymmetric(G_t const &G){
    auto p=boost::vertices(G);
    for(; p.first!=p.second; ++p.first){
        auto q=boost::adjacent_vertices(*p.first, G);
        for(; q.first!=q.second; ++q.first){
            if(boost::edge(*q.first, *p.first, G).second){
                return false;
            }
        }
    }
    return true;
}

//check if G is connected (slow? obsolete?)
template <typename G_t>
bool is_connected_undirected(G_t const &G){
    std::vector<std::set<typename boost::graph_traits<G_t>::vertex_descriptor> > components;
    get_components(G, components);

    return components.size() == 1;
}

/*
//checks if G is a tree
//TODO: #edges on directed symmetric graphs is twice the number on undirected graphs..
//TODO: additionally needs no_duplicate edges for directed graphs
template <typename G_t>
bool is_tree(G_t const &G){
    return is_connected(G) && boost::num_edges(G)+1 == boost::num_vertices(G);
}
*/

//checks if O is a permutation of V(G)
//TODO: use vec<bool>
template <typename O_t, typename G>
bool is_vertex_permutation(O_t const &O, G const &g)
{
    std::set<typename boost::graph_traits<G>::vertex_descriptor> S, V;

    auto p=boost::vertices(g);
    for(; p.first!=p.second; ++p.first){
        V.insert(*p.first);
    }

    for(auto x : O) {
        S.insert(x);
    }

    return S == V;
}

template<class G>
typename boost::graph_traits<G>::vertices_size_type degree(
        typename boost::graph_traits<G>::vertex_descriptor v,
        G const& g){
    return boost::degree(v, g);
}

template<class A, class B>
typename boost::graph_traits<boost::adjacency_list<A, B, boost::directedS> >::vertices_size_type degree(
        typename boost::graph_traits<boost::adjacency_list<A, B, boost::directedS> >::vertex_descriptor v,
        boost::adjacency_list<A, B, boost::directedS> const& g){
    return boost::out_degree(v, g);
}

} // treedec

#endif // guard

// vim:ts=8:sw=4:et
