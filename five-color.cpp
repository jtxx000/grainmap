#include <list>
#include <vector>
#include <stack>
#include <boost/pool/object_pool.hpp>
#include <unordered_map>

using namespace std;
using namespace boost;

#include <stdio.h>

enum vertex_type {s_none, s_4t, s_5t};

struct vertex;
struct edge;

typedef list<vertex*> vertex_stack;

struct edge_iterator {
  int i;
  edge* e;

  edge_iterator(vertex* v);

  edge* operator->() {
    return e;
  }
};

#define foreach_edge(edge, vertex) for (edge_iterator edg(vertex); --edg.i >=0; edg.e = edg.e->next)

typedef char vertex_color;

struct edge {
  vertex* vtx;
  edge* pos;
  edge* prev;
  edge* next;

  void remove() {
    prev->next = next;
    next->prev = prev;
  }

  void identify(edge* other) {
    if (other->vtx == vtx) {
      other->remove();
      other = other->prev;
    }

    other->next->prev = prev;
    other->next = this;
    prev->next = other->next;
    prev = other;
  }
};

struct vertex {
  edge* root_edge;
  int degree;
  vertex_type type;
  vertex_stack::iterator stack_pos;
  vertex_color color;
  bool mark;
  unordered_map<vertex*,edge*> edge_map;

  vertex() : root_edge(0), degree(0), type(s_none), color(0), mark(false) {
  }

  void add_edge(vertex* other, object_pool<edge>& pool) {
    if (edge_map.find(other) != edge_map.end())
      return;

    edge* e = pool.malloc();
    e->prev = root_edge;
    e->next = root_edge->next;
    root_edge->next->prev = e;
    root_edge->next = e;
    root_edge = e;

    edge_map.insert(pair<vertex*,edge*>(other, e));

    if (!other->edge_map.empty())
      e->pos = other->edge_map[this];
  }

  void remove() {
    printf("remove %p %d\n", this, degree);
    foreach_edge(edg, this) {
      printf("remove from edge %p %d\n", edg->vtx, edg->vtx->degree);
      if (edg->pos == edg->vtx->root_edge)
        edg->vtx->root_edge = edg->pos->next;
      edg->pos->remove();
      --edg->vtx->degree;
    }
  }

  bool adjacent_to(vertex* other) {
    foreach_edge(edg, this) {
      if (edg->vtx == other)
        return true;
    }

    return false;
  }

  void assign_color() {
    foreach_edge(edg, this) {
      printf("neighbor-col: %p %d\n", edg->vtx, edg->vtx->color);
      color |= 1<<edg->vtx->color;
    }

    color = ~color;

    for (int i=0; i<5; i++) {
      if (color & 1<<i) {
        color = i;
        printf("color found: %d\n", color);
        return;
      }
    }

    printf("default color\n");

    color = 0;
  }

  edge* find_min_edge() {
    edge* edg = root_edge;
    edge* min_edge = edg;
    int min_deg = min_edge->vtx->degree;

    for (int i=degree; --i>0;) {
      if ((edg = edg->next)->vtx->degree < min_deg) {
        min_edge = edg;
        min_deg = edg->vtx->degree;
      }
    }

    return min_edge;
  }

  void remove_from_stack(vertex_stack* s4, vertex_stack* s5) {
    switch (type) {
    case s_4t:
      s4->erase(stack_pos);
      break;
    case s_5t:
      s5->erase(stack_pos);
    }

    type = s_none;
  }

  void adjust_s5(vertex_stack* s4, vertex_stack* s5) {
    if (type != s_5t)
      return;

    foreach_edge(edg, this) {
      if (edg->vtx->degree <= 6)
        return;
    }

    remove_from_stack(s4,s5);
  }

  void pop_from(vertex_stack* s4, vertex_stack* s5) {
    remove_from_stack(s4,s5);

    if (degree <= 6) {
      foreach_edge(edge, this) {
        edg->vtx->adjust_s5(s4,s5);
      }
    }
  }

  bool distinct() {
    edge* edge = root_edge;
    int i=degree;
    bool r = true;
    for (;--i>=0; edge = edge->next) {
      vertex* v = edge->vtx;
      if (v->mark) {
        edge = edge->prev;
        r = false;
        break;
      }
      v->mark = true;
    }

    for (;++i<degree; edge = edge->prev)
      edge->vtx->mark = false;

    return r;
  }

  void push_to(vertex_stack* s4, vertex_stack* s5) {
    if (degree > 5)
      return;

    vertex_stack* cur_stack = 0;

    if (degree == 5 && distinct()) {
      foreach_edge(edg, this) {
        if (edg->vtx->degree <= 6) {
          cur_stack = s5;
          type = s_5t;
          break;
        }
      }
      if (!cur_stack) return;
    } else {
      if (type == s_4t)
        return;

      remove_from_stack(s4,s5);
      cur_stack = s4;
      type = s_4t;
    }

    cur_stack->push_front(this);
    stack_pos = cur_stack->begin();
  }
};

edge_iterator::edge_iterator(vertex* v) : i(v->degree), e(v->root_edge) {
}

class five_color {
  object_pool<edge> edge_pool;
  object_pool<vertex> vertex_pool;
  vector<vertex*> vertices;

public:
  vertex* create_vertex() {
    vertex* v = vertex_pool.malloc();
    vertices.push_back(v);
    return v;
  }

  void color(vector<vertex*>& vertices) {
    vertex_stack s4, s5;
    stack<pair<vertex*, vertex*> > sd;

    printf("size: %d\n", vertices.size());

    for (vector<vertex*>::iterator v=vertices.begin(); v != vertices.end(); ++v)
      (**v).push_to(&s4,&s5);

    for (;;) {
      while (!s4.empty()) {
        vertex* v = s4.front();
        printf("\n== pop s4 %p %d ==\n", v, v->degree);
        s4.pop_front();
        v->remove();
        sd.push(pair<vertex*,vertex*>(v,0));
        // degree has decreased by one
        foreach_edge(edg, v) {
          printf("s4 edge %p\n", edg->vtx);
          edg->vtx->push_to(&s4, &s5);
        }
      }

      if (s5.empty())
        break;

      printf("pop s5\n");

      vertex* v = s5.front();
      s5.pop_front();
      v->remove();
      sd.push(pair<vertex*,vertex*>(v,0));
      edge *v1=v->find_min_edge(), *v2=v1->next, *v3=v2->next, *v4=v3->next, *a, *b;
      if (v1->vtx->adjacent_to(v3->vtx)) {
        a = v1; b = v3;
      } else {
        a = v2; b = v4;
      }

      a->vtx->pop_from(&s4,&s5);
      b->vtx->pop_from(&s4,&s5);
      b->identify(a);
      foreach_edge(edg, v) {
        edg->vtx->push_to(&s4,&s5);
      }
      sd.push(pair<vertex*,vertex*>(a->vtx,b->vtx));
    }

    while (!sd.empty()) {
      pair<vertex*,vertex*> v(sd.top());
      printf("color %p\n", v.first);
      sd.pop();
      if (v.second)
        v.first->color = v.second->color;
      else
        v.first->assign_color();
    }
  }
};

// extern "C" {
//   struct five_color {
//     struct fc_vertex {
//       edge* edges;
//       int numEdges;
//       vertex vtx;
//     }* vertices;
//     int numVertices;
//   };

//   five_color* newFiveColor(int numVertices) {
//     five_color* fc = new five_color();
//     fc->vertices = new five_color::fc_vertex[numVertices];
//     fc->numVertices = numVertices;
//     return fc;
//   }

//   void deletefive_color(five_color* c) {
//     for (int i=0; i<c->numVertices; i++)
//       delete c->vertices[i].edges;
//     delete c->vertices;
//     delete c;
//   }

//   five_color::fc_vertex* initVertex(five_color* c, int vertex, int size) {
//     five_color::fc_vertex* v = c->vertices + vertex;
//     printf("init %p %d\n", &v->vtx, size);
//     v->edges = new edge[size];
//     v->numEdges = size;
//     v->vtx.degree = size;
//     for (int i=0; i<v->numEdges; i++) {
//       v->edges[i].next = v->edges + (i+1)%v->numEdges;
//       v->edges[i].prev = v->edges + (i-1+v->numEdges)%v->numEdges;
//     }
//     if (size)
//       v->vtx.root_edge = v->edges;
//     return v;
//   }

//   void setEdge(five_color* c, five_color::fc_vertex* fromVertex, int fromEdge, int toVertex, int toEdge) {
//     edge* e = fromVertex->edges + fromEdge;
//     five_color::fc_vertex* v = c->vertices + toVertex;
//     e->vtx = &v->vtx;
//     printf("set edge %p:%d -> %d/%p:%d\n", &fromVertex->vtx, fromEdge, toVertex, e->vtx, toEdge);
//     e->pos = v->edges + toEdge;
//   }

//   int vertexColor(five_color::fc_vertex* v) {
//     return v->vtx.color;
//   }

//   void run_five_color(five_color* c) {
//     vector<vertex*> vs;
//     vs.reserve(c->numVertices);
//     for (int i=0; i<c->numVertices; i++)
//       vs.push_back(&c->vertices[i].vtx);
//     five_color_vertices(vs);
//   }
// }


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(init) {
  vertex v;
  BOOST_CHECK(v.degree == 0);
}

void checkMarks(vertex* v) {
  foreach_edge(edg, v) {
    BOOST_CHECK(!edg->vtx->mark);
  }
}

BOOST_AUTO_TEST_CASE(distinct) {
  vertex v;
  vertex v1,v2,v3;
  edge e1,e2,e3;
  v.degree = 3;
  e1.vtx = &v1; e1.prev = &e3; e1.next = &e2;
  e2.vtx = &v2; e2.prev = &e1; e2.next = &e3;
  e3.vtx = &v3; e3.prev = &e2; e3.next = &e1;
  v.root_edge = &e1;

  BOOST_CHECK(v.distinct());
  checkMarks(&v);
  e2.vtx = &v1;
  BOOST_CHECK(!v.distinct());
  checkMarks(&v);
}

