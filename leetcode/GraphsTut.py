### Traversing a graph
import json
from pprint import pprint 
graph = {
    'a': ['b', 'c'],
    'b': ['d'],
    'c': ['e'],
    'd': ['f'],             
    'e': [],
    'f': [], 
}

"""
 /> b --> d --> f
a 
 \ > c --> e     
"""

print("\nDepth First Iterative Search")


def depthFirstIterativePrint(graph, source):

    stack = [ source ]

    while len(stack) > 0:

        tmp = stack.pop()
        print(tmp)
        for nodes in graph[tmp]:
            stack.append(nodes)

depthFirstIterativePrint(graph, 'a')

print("\nDepth First Recursive Search")

def depthFirstRecursivePrint(graph, source):

    print(source)
    for nodes in graph[source]:
        depthFirstRecursivePrint(graph, nodes)

depthFirstRecursivePrint(graph, 'a')

print("\nBreath First Search")

def breathFirstPrint(graph, source):

    stack = [ source ]

    while len(stack) > 0:

        tmp = stack.pop(0)
        print(tmp)
        for nodes in graph[tmp]:
            stack.append(nodes)

breathFirstPrint(graph, 'a')

"""
Is there a path from 1 node to another node

"""

def hasPath(graph, cur, dst):

    if cur == dst:
        return True

    for neighbour in graph[cur]:
        res = hasPath(graph, neighbour, dst)
        if res:
            return True
    
    return False

path = hasPath(graph, 'a', 'c')
print(f"\nHas path problem, path is {path}")


"""
undirected graphs

edges
"""
print("\nUndirected Graphs")

edges = [
    ["i", "j"],
    ["k", "i"],
    ["m", "k"],
    ["k", "l"],
    ["o", "n"],
]

def edgesToAdjacencyList(edges):

    graph = {}

    for edge1, edge2 in edges:

        if edge1 not in graph:
            graph[edge1] = [ edge2 ]
        else:
            graph[edge1].append(edge2)

        if edge2 not in graph:
            graph[edge2] = [ edge1 ] 
        else:
            graph[edge2].append(edge1)

    return graph

def hasPathWithSet(graph, cur, dst, visited):

    if cur == dst:
        return True
    
    if cur in visited:
        return False

    visited.add(cur)

    for neighbour in graph[cur]:
        res = hasPathWithSet(graph, neighbour, dst, visited)
        if res:
            return True
    
    return False

def undirectedGraphPath(edges, cur, dst):

    graph = edgesToAdjacencyList(edges)
    visited = set()

    return hasPathWithSet(graph, cur, dst, visited)

undi_path = undirectedGraphPath(edges, 'j', 'm')
print(f"Is there a path from undirected path between two nodes, {undi_path}")


"""
Connected Componenets Count
"""

connected_componenets_edges = {
    0: [8, 1, 5],
    1: [0],
    5: [0, 8],
    8: [0, 5],
    2: [3, 4],
    3: [2, 4],
    4: [3, 2]
}

"""
1. for a node, go till the end and whenever you are done stop and incronememtn count
"""

print("\n The connected componenets")

def explore(graph, node, visited):
    if node in visited:
        return 0
    
    visited.add(node)
    for n in graph[node]:
        explore(graph, n, visited)

    return 1


def connectedComponenetsCount(graph):
    count = 0
    visited = set()
    for n in graph:
        count += explore(graph, n, visited)
    
    return count

print(f"The number of connected components are {connectedComponenetsCount(connected_componenets_edges)}")

"""
Largest Componenet Problem
"""

largest_componenet_graph = {
    0: [8, 1, 5],
    1: [0],
    5: [0, 8],
    8: [0, 5],
    2: [3, 4],
    3: [2, 4],
    4: [3, 2]
}

