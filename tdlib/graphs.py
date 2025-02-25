from RandomGNM_250_1000 import *
from Graph import Graph
from Decomp import Decomp

#some graphs
V_P6 = range(6)
E_P6 = [(0,1),(1,2),(2,3),(3,4),(4,5)]

V_K5 = range(5)
E_K5 = [(i, j) for i in range(5) for j in range(i+1, 5)]

V_Petersen = range(10)
E_Petersen = [(0, 1), (0, 4), (0, 5), (1, 2), (1, 6), (2, 3), (2, 7), \
              (3, 4), (3, 8), (4, 9), (5, 7), (5, 8), (6, 8), (6, 9), (7, 9)]

V_Petersen_double = list(range(10))+list(range(20, 30))
E_Petersen_double = [(0, 1), (0, 4), (0, 5), (1, 2), (1, 6), (2, 3), (2, 7), \
              (3, 4), (3, 8), (4, 9), (5, 7), (5, 8), (6, 8), (6, 9), (7, 9), \
              (20,21),(20,24),(20,25),(21,22),(21,26),(22,23),(22,27), \
              (23,24),(23,28),(24,29),(25,27),(25,28),(26,28),(26,29),(27,29)]

V_Wagner = range(8)
E_Wagner = [(0, 1),(0, 4), (0, 7), (1, 2), (1, 5), (2, 3), (2, 6), (3, 4), \
            (3, 7), (4, 5), (5, 6), (6, 7)]

V_Pappus = range(18)
E_Pappus = [(0, 1), (0, 5), (0, 6), (1, 2), (1, 7), (2, 3), (2, 8), (3, 4), \
            (3, 9), (4, 5), (4, 10), (5, 11), (6, 13), (6, 17), (7, 12), \
            (7, 14), (8, 13), (8, 15), (9, 14), (9, 16), (10, 15), (10, 17), \
            (11, 12), (11, 16), (12, 15), (13, 16), (14, 17)]

#grids
V_Grid_5_5 = range(25)
E_Grid_5_5 = [(1, 0), (2, 1), (3, 2), (4, 3), (5, 0), (6, 1), (6, 5), (7, 2), \
              (7, 6), (8, 3), (8, 7), (9, 4), (9, 8), (10, 5), (11, 6), \
              (11, 10), (12, 7), (12, 11), (13, 8), (13, 12), (14, 9), (14, 13), \
              (15, 10), (16, 11), (16, 15), (17, 12), (17, 16), (18, 13), (18, 17), \
              (19, 14), (19, 18), (20, 15), (21, 16), (21, 20), (22, 17), (22, 21), \
              (23, 18), (23, 22), (24, 19), (24, 23)]


#corner cases

V_Empty = []
E_Empty = []

V_OneNode = [0]
E_OneNode = []

V_Edgeless = range(5)
E_Edgeless = []

V_Clique = range(5)
E_Clique = [(i, j) for i in range(5) for j in range(i+1, 5)]

cornercases = [[V_Empty, E_Empty], [V_OneNode, E_OneNode], [V_Edgeless, E_Edgeless], [V_Clique,  E_Clique]]


#random graphs
def randomGNP(n, p):
    import random
    V = []
    E = []
    for i in range(0, n):
        V.append(i)
        for j in range(i+1,n):
            if random.randint(0, 100) <= p*100:
                E.append((i, j))
    return V, E




#?what is this? (duplicate in treedec/tests/graph2.py)
V_Gs_at_ipo = [1,2,3,4,5,6,7,8]
E_Gs_at_ipo = [(1,2),(1,3),(1,4),(2,6),(2,7),(3,6),(3,8),(4,7),(4,8),(5,6),(5,7),(5,8)]

V_GsF__dn_ = [1,2,3,4,5,6,7]
E_GsF__dn_ = [(1,2),(1,5),(1,7),(2,6),(2,7),(3,4),(3,6),(3,7),(4,5),(4,7),(5,6)]
