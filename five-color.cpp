#include <list>
#include <vector>
#include <stack>

using namespace std;

#include <stdio.h>

enum VertexType {sNone, s4T, s5T};

struct Vertex;
struct Edge;

typedef list<Vertex*> VertexStack;

struct EdgeIterator {
  int i;
  Edge* edge;

  EdgeIterator(Vertex* v);

  Edge* operator->() {
    return edge;
  }
};

#define foreachEdge(edge, vertex) for (EdgeIterator edge(vertex); --edge.i >=0; edge.edge = edge.edge->next)

typedef char VertexColour;

struct Edge {
  Vertex* vertex;
  Edge* pos;
  Edge* prev;
  Edge* next;

  void remove() {
    prev->next = next;
    next->prev = prev;
  }

  void identify(Edge* other) {
    if (other->vertex == vertex) {
      other->remove();
      other = other->prev;
    }

    other->next->prev = prev;
    other->next = this;
    prev->next = other->next;
    prev = other;
  }
};

struct Vertex {
  Edge* rootEdge;
  int degree;
  VertexType type;
  VertexStack::iterator stackPos;
  VertexColour colour;
  bool mark;

  Vertex() : rootEdge(0), degree(0), type(sNone), colour(0), mark(false) {
  }

  void remove() {
    printf("remove %p %d\n", this, degree);
    foreachEdge(edge, this) {
      printf("remove from edge %p %d\n", edge->vertex, edge->vertex->degree);
      if (edge->pos == edge->vertex->rootEdge)
        edge->vertex->rootEdge = edge->pos->next;
      edge->pos->remove();
      --edge->vertex->degree;
    }
  }

  bool adjacentTo(Vertex* other) {
    foreachEdge(edge, this) {
      if (edge->vertex == other)
        return true;
    }

    return false;
  }

  void assignColour() {
    foreachEdge(edge, this) {
      printf("neighbor-col: %p %d\n", edge->vertex, edge->vertex->colour);
      colour |= 1<<edge->vertex->colour;
    }

    colour = ~colour;

    for (int i=0; i<5; i++) {
      if (colour & 1<<i) {
        colour = i;
        printf("colour found: %d\n", colour);
        return;
      }
    }

    printf("default colour\n");

    colour = 0;
  }

  Edge* findMinEdge() {
    Edge* edge = rootEdge;
    Edge* minEdge = edge;
    int minDeg = minEdge->vertex->degree;

    for (int i=degree; --i>0;) {
      if ((edge = edge->next)->vertex->degree < minDeg) {
        minEdge = edge;
        minDeg = edge->vertex->degree;
      }
    }

    return minEdge;
  }

  void removeFromStack(VertexStack* s4, VertexStack* s5) {
    switch (type) {
    case s4T:
      s4->erase(stackPos);
      break;
    case s5T:
      s5->erase(stackPos);
    }

    type = sNone;
  }

  void adjustS5(VertexStack* s4, VertexStack* s5) {
    if (type != s5T)
      return;

    foreachEdge(edge, this) {
      if (edge->vertex->degree <= 6)
        return;
    }

    removeFromStack(s4,s5);
  }

  void popFrom(VertexStack* s4, VertexStack* s5) {
    removeFromStack(s4,s5);

    if (degree <= 6) {
      foreachEdge(edge, this) {
        edge->vertex->adjustS5(s4,s5);
      }
    }
  }

  bool distinct() {
    Edge* edge = rootEdge;
    int i=degree;
    bool r = true;
    for (;--i>=0; edge = edge->next) {
      Vertex* v = edge->vertex;
      if (v->mark) {
        edge = edge->prev;
        r = false;
        break;
      }
      v->mark = true;
    }

    for (;++i<degree; edge = edge->prev)
      edge->vertex->mark = false;

    return r;
  }

  void pushTo(VertexStack* s4, VertexStack* s5) {
    if (degree > 5)
      return;

    VertexStack* curStack = 0;

    if (degree == 5 && distinct()) {
      foreachEdge(edge, this) {
        if (edge->vertex->degree <= 6) {
          curStack = s5;
          type = s5T;
          break;
        }
      }
      if (!curStack) return;
    } else {
      if (type == s4T)
        return;

      removeFromStack(s4,s5);
      curStack = s4;
      type = s4T;
    }

    curStack->push_front(this);
    stackPos = curStack->begin();
  }
};

EdgeIterator::EdgeIterator(Vertex* v) : i(v->degree), edge(v->rootEdge) {
}

void fiveColour(vector<Vertex*>& xs) {
  VertexStack s4, s5;
  stack<pair<Vertex*, Vertex*> > sd;

  printf("size: %d\n", xs.size());

  for (vector<Vertex*>::iterator v=xs.begin(); v != xs.end(); ++v)
    (**v).pushTo(&s4,&s5);

  for (;;) {
    while (!s4.empty()) {
      Vertex* v = s4.front();
      printf("\n== pop s4 %p %d ==\n", v, v->degree);
      s4.pop_front();
      v->remove();
      sd.push(pair<Vertex*,Vertex*>(v,0));
      // Degree has decreased by one
      foreachEdge(edge, v) {
        printf("s4 edge %p\n", edge->vertex);
        edge->vertex->pushTo(&s4, &s5);
      }
    }

    if (s5.empty())
      break;

    printf("pop s5\n");

    Vertex* v = s5.front();
    s5.pop_front();
    v->remove();
    sd.push(pair<Vertex*,Vertex*>(v,0));
    Edge *v1=v->findMinEdge(), *v2=v1->next, *v3=v2->next, *v4=v3->next, *a, *b;
    if (v1->vertex->adjacentTo(v3->vertex)) {
      a = v1; b = v3;
    } else {
      a = v2; b = v4;
    }

    a->vertex->popFrom(&s4,&s5);
    b->vertex->popFrom(&s4,&s5);
    b->identify(a);
    foreachEdge(edge, v) {
      edge->vertex->pushTo(&s4,&s5);
    }
    sd.push(pair<Vertex*,Vertex*>(a->vertex,b->vertex));
  }

  while (!sd.empty()) {
    pair<Vertex*,Vertex*> v(sd.top());
    printf("colour %p\n", v.first);
    sd.pop();
    if (v.second)
      v.first->colour = v.second->colour;
    else
      v.first->assignColour();
  }
}

extern "C" {
  struct FiveColour {
    struct FCVertex {
      Edge* edges;
      int numEdges;
      Vertex vertex;
    }* vertices;
    int numVertices;
  };

  FiveColour* newFiveColour(int numVertices) {
    FiveColour* fc = new FiveColour();
    fc->vertices = new FiveColour::FCVertex[numVertices];
    fc->numVertices = numVertices;
    return fc;
  }

  void deleteFiveColour(FiveColour* c) {
    for (int i=0; i<c->numVertices; i++)
      delete c->vertices[i].edges;
    delete c->vertices;
    delete c;
  }

  FiveColour::FCVertex* initVertex(FiveColour* c, int vertex, int size) {
    FiveColour::FCVertex* v = c->vertices + vertex;
    printf("init %p %d\n", &v->vertex, size);
    v->edges = new Edge[size];
    v->numEdges = size;
    v->vertex.degree = size;
    for (int i=0; i<v->numEdges; i++) {
      v->edges[i].next = v->edges + (i+1)%v->numEdges;
      v->edges[i].prev = v->edges + (i-1+v->numEdges)%v->numEdges;
    }
    if (size)
      v->vertex.rootEdge = v->edges;
    return v;
  }

  void setEdge(FiveColour* c, FiveColour::FCVertex* fromVertex, int fromEdge, int toVertex, int toEdge) {
    Edge* e = fromVertex->edges + fromEdge;
    FiveColour::FCVertex* v = c->vertices + toVertex;
    e->vertex = &v->vertex;
    printf("set edge %p:%d -> %d/%p:%d\n", &fromVertex->vertex, fromEdge, toVertex, e->vertex, toEdge);
    e->pos = v->edges + toEdge;
  }

  int vertexColour(FiveColour::FCVertex* v) {
    return v->vertex.colour;
  }

  void runFiveColour(FiveColour* c) {
    vector<Vertex*> vs;
    vs.reserve(c->numVertices);
    for (int i=0; i<c->numVertices; i++)
      vs.push_back(&c->vertices[i].vertex);
    fiveColour(vs);
  }
}


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(init) {
  Vertex v;
  BOOST_CHECK(v.degree == 0);
}

void checkMarks(Vertex* v) {
  foreachEdge(edge, v) {
    BOOST_CHECK(!edge->vertex->mark);
  }
}

BOOST_AUTO_TEST_CASE(distinct) {
  Vertex v;
  Vertex v1,v2,v3;
  Edge e1,e2,e3;
  v.degree = 3;
  e1.vertex = &v1; e1.prev = &e3; e1.next = &e2;
  e2.vertex = &v2; e2.prev = &e1; e2.next = &e3;
  e3.vertex = &v3; e3.prev = &e2; e3.next = &e1;
  v.rootEdge = &e1;

  BOOST_CHECK(v.distinct());
  checkMarks(&v);
  e2.vertex = &v1;
  BOOST_CHECK(!v.distinct());
  checkMarks(&v);
}

