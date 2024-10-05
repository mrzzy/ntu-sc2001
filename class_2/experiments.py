#!/usr/bin/env python
# coding: utf-8

# In[1]:


get_ipython().run_line_magic('pip', 'install -r requirements.txt')


# In[2]:


import networkx as nx
import time
import random
import numpy as np
import pandas as pd
from random import randint
from multiprocessing.pool import Pool
from itertools import combinations
from psutil import Popen, TimeoutExpired
from networkx import Graph
from tempfile import NamedTemporaryFile


# This is how to generate graphs:

# In[3]:


def random_connected_graph(n_vertex: int, p_edge:float, seed:int = 42) -> Graph:
    """Generate a random connected graph.
    
    Args:
        n_vertex: No. of vertices in the generated graph.
        p_edge: Probability of generating a random edge between any two vertices.
        seed: Seed used to seed the random number generator.
    Returns:
        A random connected graph.
    """
    g = Graph()
    random.seed(seed)
    
    # generate a spanning tree that connects all vertices
    # this ensures that the resultant random graph would be connected
    unvisited = list(range(1, n_vertex))
    random.shuffle(unvisited)
    src = 0
    while len(unvisited) > 0:
        # pop a random unvisited vertex and connect them on the graph
        dest = unvisited.pop()
        g.add_edge(src, dest, weight=randint(1, n_vertex))
        src = dest
    
    # sample all combinations of edges between two vertices to create random edges on the graph
    for edge in combinations(range(n_vertex), 2):
        if random.random() < p_edge:
            g.add_edge(*edge, weight=randint(1, n_vertex))
    
    return g


# In[4]:


nx.draw(random_connected_graph(6,0.7777777777777777, 42))


# How to run experiment trials:

# In[5]:


def trial(graph: Graph, graph_adj: str, p_queue: str, start: int, end: int, program: str = "./a.out") -> dict[str, float]:
    """Perform one trial of djistrka shortest path.

    Args:
        graph: Weighted graph to perform shortest path on.
            Weights should be set on node data with the "weight" key
        graph_adj: Graph adjcency implementation to use. Either 'matrix' or 'list'.
        p_queue: Priority Queue implementation to use. Either 'array' or 'heap'.
        start: Starting node to traverse from.
        end: End node to traverse to.
        program: Path to the executable compiled from code.cpp.
        
    Returns: Dict of expriment statistics:
        - ram: no. of bytes consumed by the program
        - runtime: wall time elapsed by start and completion of the program.
    """
    with NamedTemporaryFile("w+") as f:
        # encode trial parameters into input file
        f.write(f"{graph.number_of_nodes()} {start} {end}\n")
        for src, dest, data in graph.edges(data=True):
            # simulate undirected edge via 2 directed edges in opposite directions
            f.write(f"{src} {data['weight']} {dest}\n")
            f.write(f"{dest} {data['weight']} {src}\n")
        f.flush()

        # run program as a subprocess
        begin = time.monotonic()
        process = Popen([program, graph_adj, p_queue, f.name])
        stats = {"ram": 0}
        while process.is_running():
            # record system resource usage
            stats["time"] = time.monotonic() - begin
            stats["ram"] = max(stats["ram"], process.memory_info().vms)
            try:
                process.wait(1e-6)
            except TimeoutExpired:
                pass
        return stats
        
    


# In[17]:


def process(chunk):
    n_vertex, p_edge, graph_adj, pq = chunk
    graph = random_connected_graph(n_vertex, p_edge, seed=42)
    # run a shortest path trial from 0 -> random vertex
    stats = trial(graph, graph_adj, pq, 0, randint(0, n_vertex - 1))
    return stats | {
        "n_vertex": n_vertex,
        "p_edge": p_edge,
        "graph_adj": graph_adj,
        "priority_queue": pq,
    }


params = [
    (max(1, n_vertex), float(p_edge), graph_adj, pq)
    # 1, 50, ... 1000 vertices
    for n_vertex in range(0, 1000 + 1, 50)
    # p_edge = 0, 0.05, ... 1.0
    for p_edge in np.linspace(0, 1, 20)
    for graph_adj in ["matrix", "list"]
    for pq in ["array", "heap"]
]

random.shuffle(params)
df = pd.DataFrame(Pool().map(process, params))


# In[18]:


df


# In[23]:


df[(df["n_vertex"] == 1000) & (df["p_edge"] ==  1.0)]


# In[8]:


df.to_csv("results.csv")


# In[ ]:


##

