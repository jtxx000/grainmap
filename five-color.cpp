/* five-color.cpp
 *
 * Copyright 2011 Caleb Reach
 * 
 * This file is part of Grainmap
 *
 * Grainmap is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Grainmap is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Grainmap.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "five-color.h"

using namespace std;
using namespace boost;

#include <stdio.h>
#include <stack>
#include <assert.h>

struct five_color::edge_iterator {
  int i;
  five_color::edge* e;
  
  edge_iterator(five_color::vertex* v) : i(v->degree), e(v->root_edge) {
  }

  five_color::edge* operator->() {
    return e;
  }
};

#define foreach_edge(edge, vertex)                                  \
  for (edge_iterator edg(vertex); --edg.i >=0; edg.e = edg.e->next)

//// edge //////////////////////////////////////////////////////////////////////

void five_color::edge::remove() {
  prev->next = next;
  next->prev = prev;
}

void five_color::edge::identify(edge* other) {
  if (other->vtx == vtx) {
    other->remove();
    other = other->prev;
  }

  other->next->prev = prev;
  other->next = this;
  prev->next = other->next;
  prev = other;
}

//// vertex ////////////////////////////////////////////////////////////////////

five_color::vertex::vertex()
  : root_edge(0),
    degree(0),
    type(s_none),
    color(0),
    mark(false)
{
}

void five_color::vertex::add_edge(five_color::vertex* other,
                                  boost::object_pool<five_color::edge>& pool)
{
  if (edge_map.find(other) != edge_map.end())
    return;
  
  edge* e = pool.construct();
  e->vtx = other;
  if (root_edge) {
    e->prev = root_edge;
    e->next = root_edge->next;
    root_edge->next->prev = e;
    root_edge->next = e;
    root_edge = e;
  } else root_edge = e->next = e->prev = e;
  edge_map.insert(pair<vertex*,edge*>(other, e));

  if (!other->edge_map.empty()) {
    e->pos = other->edge_map[this];
    assert(e->pos);
    e->pos->pos = e;
  }


  degree++;
}

void five_color::vertex::remove() {
  // printf("remove %p %d\n", this, degree);
  foreach_edge(edg, this) {
    // printf("remove from edge %p %d\n", edg->vtx, edg->vtx->degree);
    if (edg->pos == edg->vtx->root_edge)
      edg->vtx->root_edge = edg->pos->next;
    edg->pos->remove();
    --edg->vtx->degree;
  }
}

bool five_color::vertex::adjacent_to(vertex* other) {
  foreach_edge(edg, this) {
    if (edg->vtx == other)
      return true;
  }

  return false;
}

void five_color::vertex::assign_color() {
  foreach_edge(edg, this) {
    // printf("neighbor-col: %p %d\n", edg->vtx, edg->vtx->color);
    color |= 1<<edg->vtx->color;
  }

  color = ~color;

  for (int i=0; i<5; i++) {
    if (color & 1<<i) {
      color = i;
      // printf("color found: %d\n", color);
      return;
    }
  }

  // printf("default color\n");

  color = 0;
}

five_color::edge* five_color::vertex::find_min_edge() {
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

void five_color::vertex::remove_from_stack(vertex_stack* s4, vertex_stack* s5) {
  switch (type) {
  case s_4t:
    s4->erase(stack_pos);
    break;
  case s_5t:
    s5->erase(stack_pos);
  }

  type = s_none;
}

void five_color::vertex::adjust_s5(vertex_stack* s4, vertex_stack* s5) {
  if (type != s_5t)
    return;

  foreach_edge(edg, this) {
    if (edg->vtx->degree <= 6)
      return;
  }

  remove_from_stack(s4,s5);
}

void five_color::vertex::pop_from(vertex_stack* s4, vertex_stack* s5) {
  remove_from_stack(s4,s5);

  if (degree <= 6) {
    foreach_edge(edge, this) {
      edg->vtx->adjust_s5(s4,s5);
    }
  }
}

bool five_color::vertex::distinct() {
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

void five_color::vertex::push_to(vertex_stack* s4, vertex_stack* s5) {
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

//// public api ////////////////////////////////////////////////////////////////

five_color::vertex* five_color::create_vertex() {
  vertex* v = vertex_pool.construct();
  vertices.push_back(v);
  return v;
}

void five_color::add_edge(vertex* from, vertex* to) {
  from->add_edge(to, edge_pool);
}

void five_color::color() {
  five_color::vertex::vertex_stack s4, s5;
  stack<pair<vertex*, vertex*> > sd;

  // printf("size: %d\n", vertices.size());

  for (vector<vertex*>::iterator v=vertices.begin(); v != vertices.end(); ++v)
    (**v).push_to(&s4,&s5);

  for (;;) {
    while (!s4.empty()) {
      vertex* v = s4.front();
      // printf("\n== pop s4 %p %d ==\n", v, v->degree);
      s4.pop_front();
      v->remove();
      sd.push(pair<vertex*,vertex*>(v,0));
      // degree has decreased by one
      foreach_edge(edg, v) {
        // printf("s4 edge %p\n", edg->vtx);
        edg->vtx->push_to(&s4, &s5);
      }
    }

    if (s5.empty())
      break;

    // printf("pop s5\n");

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
    // printf("color %p\n", v.first);
    sd.pop();
    if (v.second)
      v.first->color = v.second->color;
    else
      v.first->assign_color();
  }
}
