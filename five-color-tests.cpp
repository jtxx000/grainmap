#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include "five-color.cpp"
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

BOOST_AUTO_TEST_CASE(add_edge) {
  five_color fc;
  vertex *v1=fc.create_vertex(), *v2=fc.create_vertex();
  fc.add_edge(v1, v2);
  fc.add_edge(v2, v1);
  BOOST_CHECK(v1->root_edge->pos == v2->root_edge);
  BOOST_CHECK(v2->root_edge->pos == v1->root_edge);
}

BOOST_AUTO_TEST_CASE(color) {
  five_color fc;
  vertex *v1=fc.create_vertex(), *v2=fc.create_vertex();
  fc.add_edge(v1, v2);
  fc.add_edge(v2, v1);
  fc.color();
  BOOST_CHECK(v1->color != v2->color);
}
