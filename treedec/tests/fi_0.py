
from treedec import _graph as gr
from treedec.greedy import fi

E = [(1,2),(2,1),(2,3),(3,2)]
EU = [(1,2),(2,3)]

g = gr._balvvd(E, 4)
print(g)
a = fi(g)
print(a)
a.do_it()
t = a.get_ordering()
print(t)
# t = a.get_tree_decomposition()

g = gr._balvvu(EU, 4)
print(g)
a = fi(g)
a.do_it()
print(a)
t = a.get_ordering()
print(t)
# t = a.get_tree_decomposition()

g = gr._balsvd(E, 4)
print(g)
a = fi(g)
a.do_it()
print(a)
t = a.get_ordering()
print(t)
# t = a.get_tree_decomposition()

g = gr._balsvu(EU, 4)
print(g)
a = fi(g)
a.do_it()
print(a)
t = a.get_ordering()
print(t)
# t = a.get_tree_decomposition()
