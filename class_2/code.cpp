/*
 * SC2001
 * Example Class 2
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <forward_list>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#define UINT_MAX std::numeric_limits<uint32_t>::max()

/** Defines a vertex on a graph */
using Vertex = uint32_t;

/** Defines a vertex with an associated distance */
struct DistantVertex {
  Vertex vertex;
  uint32_t distance;

  bool operator==(const DistantVertex &other) const {
    return vertex == other.vertex;
  }

  bool operator>(const DistantVertex &other) const {
    return distance > other.distance;
  }
};

/* Defines a weighted edge between src & dest vertices on a graph. */
struct Edge {
  Vertex src;
  uint32_t weight;
  Vertex dest;
};

/*
 * Priority Queue interface.
 */
class PriorityQueue {
public:
  /** Get the minimum value */
  virtual DistantVertex min() const = 0;
  /** Current no. of elements in the Priority Queue */
  virtual uint32_t size() const = 0;
  /** Add a value into the Priority Queue */
  virtual void enqueue(DistantVertex value) = 0;
  /** Get and remove the minimum value from the Priority Queue */
  virtual DistantVertex dequeue() = 0;
  /** reduce the old vertex distance with lower distance */
  virtual void reduce(DistantVertex old, DistantVertex replacement) = 0;
  virtual ~PriorityQueue() = default;
};

/* Array implementation of a Priority Queue */
class ArrayPQ : public PriorityQueue {
private:
  // use vector to implement array to simplify implementation
  // items are stored in descending order, with the last element also being the
  // minimum element.
  std::vector<DistantVertex> items;

public:
  // O(1)
  DistantVertex min() const override { return items.back(); }

  uint32_t size() const override { return items.size(); }

  // O(n)
  void enqueue(DistantVertex value) override {
    // dequeue elements smaller than value
    std::stack<DistantVertex> smaller;
    while (size() > 0 && value > min()) {
      smaller.push(dequeue());
    }
    // insert value in sorted order
    items.push_back(value);
    // enqueue elements smaller than value back in
    while (smaller.size() > 0) {
      items.push_back(smaller.top());
      smaller.pop();
    }
  }

  // O(1)
  DistantVertex dequeue() override {
    DistantVertex value = items.back();
    items.pop_back();
    return value;
  }

  // O(nlogn)
  void reduce(DistantVertex old, DistantVertex replacement) override {
    // dequeue until we locate old value
    std::stack<DistantVertex> dequeued;
    while (size() > 0 && min() != old) {
      dequeued.push(dequeue());
    }
    // push the replacement value onto the priority queue
    enqueue(replacement);

    // shift dequeued elements back onto the priority queue
    while (dequeued.size() > 0) {
      enqueue(dequeued.top());
      dequeued.pop();
    }
  }
};

/* Min Heap implementation of a Priority Queue */
class HeapPQ : public PriorityQueue {
private:
  // items stored in min heap ordering
  std::vector<DistantVertex> items;
  // lookup table to enable fast O(1) lookup of position of vertex within items
  std::unordered_map<Vertex, int> lookup;
  // comparator used to order heap
  // greater<uint32_t> ensures that minimum element stays on top in the stdlib's
  // max heap implementation
  std::greater<DistantVertex> cmp;

public:
  // O(1)
  DistantVertex min() const override { return items[0]; }

  uint32_t size() const override { return items.size(); }

  // O(log(n))
  void enqueue(DistantVertex value) override {
    // add item and fix heap invariant
    items.push_back(value);
    std::push_heap(items.begin(), items.end(), cmp);
    // create index mapping in lookup table
    lookup[value.vertex] = items.size() - 1;
  }

  // O(log(n))
  DistantVertex dequeue() override {
    // swap out min element and fix heap invariant
    std::pop_heap(items.begin(), items.end(), cmp);

    // remove vertex from collections
    DistantVertex value = items.back();
    lookup.erase(value.vertex);
    items.pop_back();
    return value;
  }

  // O(log(n))
  void reduce(DistantVertex old, DistantVertex replacement) override {
    if (old.distance < replacement.distance) {
      throw new std::logic_error("Expected vertex replacement distance to be "
                                 "smaller than old distance");
    }

    // nonexisting old item, create intesad
    if (!lookup.contains(old.vertex)) {
      return enqueue(replacement);
    }

    // update distance for old item
    const int index = lookup[old.vertex];
    items[index].distance = replacement.distance;

    // fix heap invariant upwards since distance has decreased for vertex
    fixHeapUp(index);
  }

private:
  /**
   * Fix heap invariant upwards from index -> 0 (root).
   * Specialised implementation is needed as both items vector & lookup table
   * have to be updated.
   */
  void fixHeapUp(int index) {
    while (index > 0 && items[index].distance < items[parent(index)].distance) {
      // swap index & parent positions to fix heap invariant
      lookup[items[index].vertex] = parent(index);
      lookup[items[parent(index)].vertex] = index;
      std::swap(items[index], items[parent(index)]);

      index = parent(index);
    }
  }

  /** Get heap parent index of given index */
  int parent(int index) { return (index - 1) / 2; }
};

/*
 * Directed Weighted Graph interface.
 */
class Graph {
public:
  /**
   * Initialises a graph with vertex 0 to n_vertices (exclusive) & given edges.
   * Overrides any previously stored vertices & edges.
   * Infers vertices from the src & dest of the given edges.
   */
  virtual void init(uint32_t n_vertices, std::vector<Edge> edges) = 0;

  /** Get the number of vertices in this graph. */
  virtual uint32_t n_vertices() const = 0;

  /** Get an list to all edges from with the given source 'src' vertex. */
  virtual std::forward_list<Edge> connected(Vertex src) const = 0;

  virtual ~Graph() = default;
};

/*
 * Adjacent Matrix implementation of a Graph.
 */
class AdjMatrix : public Graph {
private:
  // 2D matrix tracking connections on a graph:
  // - vertices are not connected: matrix[i, j] = uint32_t max
  // - vertices are connected with weight w: matrix[i,j] = w
  std::vector<std::vector<uint32_t>> matrix;

public:
  void init(uint32_t n_vertices, std::vector<Edge> edges) override {
    // uint32_t max used as sentinel value for: "not connected"
    matrix = std::vector(n_vertices, std::vector(n_vertices, UINT_MAX));
    for (Edge e : edges) {
      matrix[e.src][e.dest] = e.weight;
    }
  }

  uint32_t n_vertices() const override { return matrix.size(); }

  // O(n)
  std::forward_list<Edge> connected(Vertex src) const override {
    if (src >= matrix.size()) {
      throw std::logic_error("src vertex out of bounds");
    }

    // collect connected edges for src vertex
    std::forward_list<Edge> edges;
    for (uint32_t dest = 0; dest < n_vertices(); dest++) {
      if (matrix[src][dest] != UINT_MAX) {
        edges.push_front(Edge{src, matrix[src][dest], dest});
      }
    }

    return edges;
  }
};

/*
 * Adjacent List implementation of a Graph.
 */
class AdjList : public Graph {
private:
  // array of adjacency lists tracking connected edges of each vertex.
  // neighbours[v] is a list of edges to neighbouring vertices
  std::vector<std::forward_list<Edge>> neighbours;

public:
  void init(uint32_t n_vertices, std::vector<Edge> edges) override {
    neighbours = std::vector(n_vertices, std::forward_list<Edge>());
    for (Edge e : edges) {
      neighbours[e.src].push_front(e);
    }
  }

  uint32_t n_vertices() const override { return neighbours.size(); }

  // O(1)
  std::forward_list<Edge> connected(Vertex src) const override {
    if (src >= n_vertices()) {
      throw std::logic_error("src vertex out of bounds");
    }
    return neighbours[src];
  }
};

/*
 * Find the shortest path from start to end vertex on the given graph.
 */
template <typename PQ>
std::forward_list<Vertex> find_shortest(const Graph &graph, Vertex start,
                                        Vertex end) {
  // distance[v] tracks shortest distance from start vertex to vertex v
  std::vector<uint32_t> distance(graph.n_vertices(), UINT_MAX);
  distance[start] = 0;
  // previous[v] tracks previous vertex in shortest path to vertex v
  // or uint32_t if no such shortest path exists
  std::vector<Vertex> previous(graph.n_vertices(), UINT_MAX);
  // priority queue of explorable vertices
  PQ pq = PQ();
  PriorityQueue &explorable = pq;
  explorable.enqueue(DistantVertex{start, 0});
  // set of already explored vertices
  std::vector<bool> is_explored(graph.n_vertices(), false);

  while (explorable.size() > 0) {
    auto [vertex, min_dist] = explorable.dequeue();
    // mark current vertex as explored
    is_explored[vertex] = true;

    // check if we found a path to the ending vertex
    if (vertex == end) {
      // reconstruct shortest path to ending vertex
      int v = end;
      std::forward_list<Vertex> path;
      while (v != start) {
        path.push_front(v);
        v = previous[v];
      }
      path.push_front(start);
      return path;
    }

    // enqueue neighbouring vertices for exploration
    for (Edge edge : graph.connected(vertex)) {
      Vertex neighbour = edge.dest;

      // only consider unexplored vertices
      if (is_explored[neighbour]) {
        continue;
      }

      // compute distance to neighbour via vertex
      uint32_t via_distance = min_dist + edge.weight;

      if (via_distance < distance[neighbour]) {
        // found shorter distance: update currently known shortest distance to
        // neighbour
        explorable.reduce(DistantVertex{neighbour, distance[neighbour]},
                          DistantVertex{neighbour, via_distance});
        // update distance[v]
        distance[neighbour] = via_distance;
        // track vertex is prior vertex in shortest path
        previous[neighbour] = vertex;
      }
    }
  }

  throw std::logic_error("No path exists from start -> end");
}

int main(int argc, char *argv[]) {
  if (argc <= 3) {
    std::cout
        << "Usage: class_2 <graph> <priority queue> <input>\n"
           "  <graph> Graph adjacency implementation to use. Either 'matrix' "
           "or 'list'.\n"
           "  <priority queue> Priority Queue implementation to use. Either "
           "'array' or 'heap'.\n"
           "  <input> Path to shortest path input in the format:\n"
           "     <no. of vertices> <start vertex> <end vertex>\n"
           "     <src vertex> <weight> <dest vertex>\n"
           "     <src vertex> <weight> <dest vertex>\n"
           "     ..."
        << std::endl;
    return 1;
  }

  // parse shortest path problem parameters from input file
  std::ifstream input(argv[3]);
  uint32_t n_vertices;
  Vertex start, end;
  input >> n_vertices >> start >> end;

  std::vector<Edge> edges;
  Edge edge;
  while (input >> edge.src >> edge.weight >> edge.dest) {
    edges.push_back(edge);
  }

  // initialise graph based on selected implementation
  Graph *graph = (std::string(argv[1]) == "matrix")
                     ? static_cast<Graph *>(new AdjMatrix())
                     : static_cast<Graph *>(new AdjList());
  graph->init(n_vertices, edges);

  // perform shortest path search using priority queue of selected
  // implementation
  auto path = (std::string(argv[2]) == "array")
                  ? find_shortest<ArrayPQ>(*graph, start, end)
                  : find_shortest<HeapPQ>(*graph, start, end);

  // output shortest path
  std::ostringstream path_str;
  for (int vertex : path) {
    path_str << vertex << " ";
  }
  std::cout << "shortest: " << path_str.str() << std::endl;
}
