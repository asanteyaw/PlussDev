#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include "MSharp/SnapPtr.h"

using pluss::msharp::make_snap;
using pluss::msharp::RefCounted;
using pluss::msharp::SnapPtr;

struct Foo : RefCounted {
  int v;
  explicit Foo(int x) : v(x) {}
};

TEST_CASE("SnapPtr retain/release and adopt", "[snapptr]") {
  auto p = make_snap<Foo>(3);
  REQUIRE(p.use_count() == 1);
  {
    SnapPtr<Foo> q = p;
    REQUIRE(p.use_count() == 2);
    REQUIRE(q->v == 3);
  }
  REQUIRE(p.use_count() == 1);
}
