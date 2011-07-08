#include <list>
#include <vector>
#include <boost/pool/object_pool.hpp>
#include <unordered_map>

class five_color {
private:
  class edge;
  struct edge_iterator;

public:
  class vertex {
  public:
    typedef char vertex_color;
    vertex_color color;
    vertex();
    
  private:
    typedef std::list<vertex*> vertex_stack;
    enum vertex_type {s_none, s_4t, s_5t};

    friend class edge;
    friend class edge_iterator;
    friend class five_color;

    edge* root_edge;
    int degree;
    vertex_type type;
    vertex_stack::iterator stack_pos;
    bool mark;
    std::unordered_map<vertex*,edge*> edge_map;

    void add_edge(vertex* other, boost::object_pool<edge>& pool);
    void remove();
    bool adjacent_to(vertex* other);
    void assign_color();
    edge* find_min_edge();
    void remove_from_stack(vertex_stack* s4, vertex_stack* s5);
    void adjust_s5(vertex_stack* s4, vertex_stack* s5);
    void pop_from(vertex_stack* s4, vertex_stack* s5);
    bool distinct();

    void push_to(vertex_stack* s4, vertex_stack* s5);
  };

  vertex* create_vertex();
  void add_edge(vertex* from, vertex* to);
  void color();

private:
  class edge {
    vertex* vtx;
    edge* pos;
    edge* prev;
    edge* next;

    friend class vertex;
    friend class five_color;

    void remove();
    void identify(edge* other);
  };

  boost::object_pool<edge> edge_pool;
  boost::object_pool<vertex> vertex_pool;
  std::vector<vertex*> vertices;
};
