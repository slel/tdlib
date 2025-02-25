// Copyright (C) 2016 Lukas Larisch
// Author: Lukas Larisch
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

#ifndef CONVENIENCE
#define CONVENIENCE

// TODO: inclusion is upside down.
#include "generic_base.hpp"
#include "generic_elimination_search_configs.hpp"
#include "preprocessing.hpp"

#ifdef HAVE_GALA_GRAPH_H
#include <gala/boost.h>
#endif

#include <iostream>
#include <algorithm>    // std::random_shuffle
#include <vector>
#include <ctime>
#include <cstdlib>


namespace treedec{

template <typename G_t>
void generic_elimination_search_CFG1(G_t const &G, unsigned max_nodes, unsigned max_orderings)
{
    gen_search::configs::CFG_DFS_1<G_t, algo::default_config>
       generic_elim_DFS_test (G);

    generic_elim_DFS_test.set_max_nodes_generated(max_nodes);
    generic_elim_DFS_test.set_max_orderings_generated(max_orderings);

    generic_elim_DFS_test.do_it();

#ifndef NDEBUG
    G_t H(G);
    size_t A=generic_elim_DFS_test.global_upper_bound_bagsize();
    size_t B=treedec::get_bagsize_of_elimination_ordering(H, generic_elim_DFS_test.ordering());
    assert(A == B);
#endif
}

template <typename G_t>
void generic_elimination_search_CFG2(G_t const &G, unsigned max_nodes, unsigned max_orderings)
{

    typedef std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> ord_type;
    ord_type ordering(boost::num_vertices(G));
    ord_type cur_ordering(boost::num_vertices(G));

    std::vector<BOOL> active(boost::num_vertices(G), true);

    gen_search::configs::CFG_DFS_2<G_t, algo::default_config>
      generic_elim_DFS_test (G);

    generic_elim_DFS_test.set_max_nodes_generated(max_nodes);
    generic_elim_DFS_test.set_max_orderings_generated(max_orderings);

    generic_elim_DFS_test.do_it();

#ifndef NDEBUG
    G_t H(G);
    size_t A=generic_elim_DFS_test.global_upper_bound_bagsize();
    size_t B=treedec::get_bagsize_of_elimination_ordering(H, generic_elim_DFS_test.ordering());
    assert(A == B);
#endif
}


template <typename G_t>
void generic_elimination_search_CFG3(G_t const &G, unsigned max_nodes, unsigned max_orderings)
{
    gen_search::configs::CFG_DFS_3<G_t, algo::default_config>
       generic_elim_DFS_test (G);

    generic_elim_DFS_test.set_max_nodes_generated(max_nodes);
    generic_elim_DFS_test.set_max_orderings_generated(max_orderings);

    generic_elim_DFS_test.do_it();

#ifndef NDEBUG
    G_t H(G);
    size_t A=generic_elim_DFS_test.global_upper_bound_bagsize();
    size_t B=treedec::get_bagsize_of_elimination_ordering(H, generic_elim_DFS_test.ordering());
    assert(A == B);
#endif
}


template <typename G_t>
void generic_elimination_search_p17(G_t &G, unsigned max_nodes, unsigned max_orderings)
{
    std::cout << "edges before PP: " << boost::num_edges(G) << std::endl;
    std::cout << "vertices before PP: " << boost::num_vertices(G) << std::endl;

    impl::preprocessing<G_t> PP(G);
    PP.do_it();

    std::vector<size_t> m;

    G_t H;
    PP.get_subgraph_copy(H, m);

    std::cout << "PP lb: " << PP.get_treewidth() << std::endl;

    if(boost::num_vertices(H) == 0){
        std::cout << "fully reduced by PP!" << std::endl;
        return;
    }

    std::cout << "edges after PP: " << boost::num_edges(H) << std::endl;
    std::cout << "vertices after PP: " << boost::num_vertices(H) << std::endl;

    typedef std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> ord_type;
    ord_type ordering(boost::num_vertices(H));
    ord_type cur_ordering(boost::num_vertices(H));

    std::vector<BOOL> active(boost::num_vertices(H), true);

    gen_search::configs::CFG_DFS_p17<G_t, algo::default_config>
       generic_elim_DFS_test (H /* ... more? */);

    generic_elim_DFS_test.set_max_nodes_generated(max_nodes);
    generic_elim_DFS_test.set_max_orderings_generated(max_orderings);

    generic_elim_DFS_test.do_it();

#ifndef NDEBUG
    {
    G_t H;
    PP.get_subgraph_copy(H, m);
    size_t A=generic_elim_DFS_test.global_upper_bound_bagsize();
    size_t B=treedec::get_bagsize_of_elimination_ordering(H, generic_elim_DFS_test.ordering());
    assert(A == B);
    }
#endif
}

template <typename G_t>
void generic_elimination_search_p17_jumper(G_t &G, unsigned max_nodes, unsigned max_orderings)
{
    clock_t start, end;

    start = clock();


    std::cout << "vertices before PP: " << boost::num_vertices(G) << std::endl;
    std::cout << "edges before PP: " << boost::num_edges(G) << std::endl;

    impl::preprocessing<G_t> PP(G);
    PP.do_it();

    std::vector<boost::tuple<
        typename treedec_traits<typename treedec_chooser<G_t>::type>::vd_type,
        typename treedec_traits<typename treedec_chooser<G_t>::type>::bag_type
         > > bags;

    PP.get_bags(bags);

    G_t G2;
    PP.get_graph(G2);

    std::cout << "PP lb: " << PP.get_treewidth() << std::endl;


    // valgrind is unhappy here when using
    // boost::adjacency_list<boost::setS, boost::vecS>.
    // looks like a boost bug.
    bool cond = true;
    while(cond){
        cond = false;
        typename boost::graph_traits<G_t>::vertex_iterator vIt, vEnd;
        for(boost::tie(vIt, vEnd) = boost::vertices(G2); vIt != vEnd; vIt++){
            if(boost::out_degree(*vIt, G2) == 0){
                boost::remove_vertex(*vIt, G2);
                cond = true;
                break;
            }
        }
    }

    if(boost::num_vertices(G2) == 0){
        std::cout << "fully reduced by PP!" << std::endl;

        end = clock();
        std::cout << "Process took " << (double(end - start) / CLOCKS_PER_SEC) << " seconds" << std::endl;
        return;
    }

    std::cout << "vertices after PP: " << boost::num_vertices(G2) << std::endl;
    std::cout << "edges after PP: " << boost::num_edges(G2) << std::endl;


    typedef std::vector<typename boost::graph_traits<G_t>::vertex_descriptor> ord_type;
    ord_type ordering(boost::num_vertices(G2));
    ord_type cur_ordering(boost::num_vertices(G2));

    std::vector<BOOL> active(boost::num_vertices(G2), true);

/*
#ifdef HAVE_GALA_NOTYET
    typedef gala::graph<std::vector, std::vector, uint32_t> ssg_vec_vec32i;

    typedef G_t Underlying_t;
    typedef ssg_vec_vec32i  Overlay_t;

    overlay<Underlying_t, Overlay_t> olay(G2, active);
#else

//    gen_search::overlay<Underlying_t, Overlay_t> olay(G);
#endif
*/

    gen_search::configs::CFG_DFS_p17<G_t, algo::default_config>
       generic_elim_DFS_test (G2 /* ... more? */);

    generic_elim_DFS_test.set_max_nodes_generated(max_nodes);
    generic_elim_DFS_test.set_max_orderings_generated(max_orderings);

    generic_elim_DFS_test.do_it();

    unsigned lb=generic_elim_DFS_test.global_lower_bound_bagsize();
    unsigned ub=generic_elim_DFS_test.global_upper_bound_bagsize();
    ord_type best=generic_elim_DFS_test.ordering();

    if(lb == ub){
        end = clock();
        std::cout << "Process took " << (double(end - start) / CLOCKS_PER_SEC) << " seconds" << std::endl;
        return;
    }

    while(true){
        gen_search::configs::CFG_DFS_p17_2<G_t, algo::default_config>
           generic_elim_DFS_test2 (G2);

        generic_elim_DFS_test2.set_max_nodes_generated(max_nodes);
        generic_elim_DFS_test2.set_max_orderings_generated(max_orderings);

        generic_elim_DFS_test2.set_lb(lb);
        generic_elim_DFS_test2.set_ub(ub);
        generic_elim_DFS_test2.set_best_ordering(best);

        generic_elim_DFS_test2.do_it();

        lb=generic_elim_DFS_test2.global_lower_bound_bagsize();
        if(generic_elim_DFS_test2.global_upper_bound_bagsize() == ub){
            end = clock();
            std::cout << "Process took " << (double(end - start) / CLOCKS_PER_SEC) << " seconds" << std::endl;
            return;
        }
        ub=generic_elim_DFS_test2.global_upper_bound_bagsize();
        best=generic_elim_DFS_test2.ordering();
    }
    end = clock();
    std::cout << "Process took " << (double(end - start) / CLOCKS_PER_SEC) << " seconds" << std::endl;

#ifndef NDEBUG
    G_t H(G2);
    size_t A=ub;
    size_t B=treedec::get_bagsize_of_elimination_ordering(H, best);
    assert(A == B);
#endif
}

} //namespace treedec



#endif //guard

// vim:ts=8:sw=4:et
