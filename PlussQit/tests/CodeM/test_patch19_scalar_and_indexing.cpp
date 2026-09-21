#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("scalar operators", "[scalar]") {
  Series a = pluss::series::full({4}, 2.0, ScalarType::Float32);
  Series b = a.add(3.0);
  auto* bp = reinterpret_cast<const float*>(b.raw_data());
  REQUIRE(bp[0] == 5.0f);

  a.add_(1.0);
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 3.0f);

  Series m = a.gt(2.0);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
}

TEST_CASE("nonzero/take/put", "[index]") {
  Series x = pluss::series::zeros({5}, ScalarType::Int64);
  auto* xp = reinterpret_cast<std::int64_t*>(x.raw_data());
  xp[1] = 7; xp[3] = 9;

  Series nz = x.ne(0.0).nonzero(); // indices
  REQUIRE(nz.dim() == 2);

  // take
  Series idx = pluss::series::zeros({2}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  ip[0]=1; ip[1]=3;
  Series t = x.take(idx);
  auto* tp = reinterpret_cast<const std::int64_t*>(t.raw_data());
  REQUIRE(tp[0] == 7);
  REQUIRE(tp[1] == 9);

  Series src = pluss::series::full({2}, 1.0, ScalarType::Int64);
  x = x.put(idx, src, false);
  auto* outp = reinterpret_cast<const std::int64_t*>(x.raw_data());
  REQUIRE(outp[1] == 1);
  REQUIRE(outp[3] == 1);
}

TEST_CASE("Series::pos bounds-checked scalar selection", "[index][pos]") {
  Series s = pluss::make_series({1.0, 2.0, 3.0});
  const pluss::codem::SeriesBase& b = s;

  Series picked = s.pos(1);
  REQUIRE(picked.dim() == 0);
  REQUIRE(picked.numel() == 1);
  REQUIRE(picked.item<double>() == 2.0);

  Series picked_from_base = b.pos(2);
  REQUIRE(picked_from_base.dim() == 0);
  REQUIRE(picked_from_base.item<double>() == 3.0);

  // Negative indices are Python-style (pos(-1) == the last element), not
  // an error -- pos() itself intentionally does `if (index < 0) index +=
  // size`, matching the same convention pluss::Slicing::SliceFrom(-1,-1)
  // uses elsewhere. Only an index that is STILL negative after that
  // wraparound (or positive and out of range) should throw.
  REQUIRE(s.pos(-1).item<double>() == 3.0);
  REQUIRE_THROWS(s.pos(-4));
  REQUIRE_THROWS(s.pos(3));

  Series m = pluss::series::ones({2, 2}, ScalarType::Float64);
  REQUIRE_THROWS(m.pos(0));
}

TEST_CASE("Series::pos(row, col) bounds-checked scalar selection", "[index][pos]") {
  Series m = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
  const pluss::codem::SeriesBase& b = m;

  Series picked = m.pos(1, 0);
  REQUIRE(picked.dim() == 0);
  REQUIRE(picked.numel() == 1);
  REQUIRE(picked.item<double>() == 3.0);

  Series picked_from_base = b.pos(0, 1);
  REQUIRE(picked_from_base.dim() == 0);
  REQUIRE(picked_from_base.item<double>() == 2.0);

  // Same Python-style negative-index convention as pos(index) above.
  REQUIRE(m.pos(-1, 0).item<double>() == 3.0);
  REQUIRE(m.pos(0, -1).item<double>() == 2.0);
  REQUIRE(m.pos(-1, -1).item<double>() == 4.0);
  REQUIRE_THROWS(m.pos(-3, 0));
  REQUIRE_THROWS(m.pos(0, -3));
  REQUIRE_THROWS(m.pos(2, 0));
  REQUIRE_THROWS(m.pos(0, 2));

  Series v = pluss::make_series({1.0, 2.0, 3.0});
  REQUIRE_THROWS(v.pos(0, 0));
}

TEST_CASE("Series::pos(i, j, k) bounds-checked scalar selection", "[index][pos]") {
  Series t = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
      {2, 2, 2});
  const pluss::codem::SeriesBase& b = t;

  Series picked = t.pos(1, 0, 1);
  REQUIRE(picked.dim() == 0);
  REQUIRE(picked.numel() == 1);
  REQUIRE(picked.item<double>() == 6.0);

  Series picked_from_base = b.pos(0, 1, 0);
  REQUIRE(picked_from_base.dim() == 0);
  REQUIRE(picked_from_base.item<double>() == 3.0);

  // Same Python-style negative-index convention as pos(index) above.
  REQUIRE(t.pos(-1, 0, 0).item<double>() == 5.0);
  REQUIRE(t.pos(0, -1, 0).item<double>() == 3.0);
  REQUIRE(t.pos(0, 0, -1).item<double>() == 2.0);
  REQUIRE_THROWS(t.pos(-3, 0, 0));
  REQUIRE_THROWS(t.pos(0, -3, 0));
  REQUIRE_THROWS(t.pos(0, 0, -3));
  REQUIRE_THROWS(t.pos(2, 0, 0));
  REQUIRE_THROWS(t.pos(0, 2, 0));
  REQUIRE_THROWS(t.pos(0, 0, 2));

  Series m = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
  REQUIRE_THROWS(m.pos(0, 0, 0));
}

TEST_CASE("Series::extract supports python-style range slicing", "[index][extract]") {
  Series y = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3});

  // Equivalent intent to torch: y.slice(1, 1, y.size(1))
  Series tail_cols = y.extract(pluss::Slicing::SliceAll(), {1, y.sizes()[1]});
  REQUIRE(tail_cols.dim() == 2);
  REQUIRE(tail_cols.sizes()[0] == 2);
  REQUIRE(tail_cols.sizes()[1] == 2);
  REQUIRE(tail_cols.pos(0, 0).item<double>() == 2.0);
  REQUIRE(tail_cols.pos(0, 1).item<double>() == 3.0);
  REQUIRE(tail_cols.pos(1, 0).item<double>() == 5.0);
  REQUIRE(tail_cols.pos(1, 1).item<double>() == 6.0);

  // Reverse columns with negative step.
  Series rev_cols = y.extract(pluss::Slicing::SliceAll(), pluss::Slicing::SliceFrom(-1, -1));
  REQUIRE(rev_cols.sizes()[0] == 2);
  REQUIRE(rev_cols.sizes()[1] == 3);
  REQUIRE(rev_cols.pos(0, 0).item<double>() == 3.0);
  REQUIRE(rev_cols.pos(0, 1).item<double>() == 2.0);
  REQUIRE(rev_cols.pos(0, 2).item<double>() == 1.0);

  // 3-D traversal.
  Series t = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
      {2, 2, 2});
  Series u = t.extract({1, 2}, {0, 2}, {0, 2});
  REQUIRE(u.dim() == 3);
  REQUIRE(u.sizes()[0] == 1);
  REQUIRE(u.sizes()[1] == 2);
  REQUIRE(u.sizes()[2] == 2);
  REQUIRE(u.pos(0, 0, 0).item<double>() == 5.0);
  REQUIRE(u.pos(0, 1, 1).item<double>() == 8.0);

  REQUIRE_THROWS(y.extract(pluss::Slicing::SliceAll(),
                           pluss::Slicing::SliceAll(),
                           {0, 1}));
  REQUIRE_THROWS(y.extract(pluss::Slicing::SliceAll(),
                           pluss::Slicing::SliceAll(),
                           pluss::Slicing::SliceAll(),
                           {0, 1}));
  REQUIRE_THROWS(y.extract(pluss::Slicing::SliceAll(), {0, 2, 0}));
}

TEST_CASE("Series::size(dim) returns int64_t dimension extent", "[index][shape]") {
  Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
      {1, 2, 3});

  REQUIRE(x.size(0) == 1);
  REQUIRE(x.size(1) == 2);
  REQUIRE(x.size(2) == 3);
  REQUIRE(x.size(-1) == 3);
  REQUIRE(x.size(-2) == 2);
  REQUIRE_THROWS(x.size(3));
  REQUIRE_THROWS(x.size(-4));
}

TEST_CASE("Series::extract supports ellipsis/new-axis/scalar+range/advanced", "[index][extract][parity]") {
  Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0},
      {2, 2, 2});

  // mixed scalar + range: dim-drop on scalar index
  Series a = x.extract(1, pluss::Slicing::SliceAll(), {0, 2, 2});
  REQUIRE(a.dim() == 2);
  REQUIRE(a.sizes()[0] == 2);
  REQUIRE(a.sizes()[1] == 1);
  REQUIRE(a.pos(0, 0).item<double>() == 5.0);
  REQUIRE(a.pos(1, 0).item<double>() == 7.0);

  // ellipsis
  Series b = x.extract(pluss::Slicing::Ellipsis(), 1);
  REQUIRE(b.dim() == 2);
  REQUIRE(b.sizes()[0] == 2);
  REQUIRE(b.sizes()[1] == 2);
  REQUIRE(b.pos(0, 0).item<double>() == 2.0);
  REQUIRE(b.pos(1, 1).item<double>() == 8.0);

  // new axis insertion
  Series c = x.extract(pluss::Slicing::NewAxis(), 1, pluss::Slicing::Ellipsis());
  REQUIRE(c.dim() == 3);
  REQUIRE(c.sizes()[0] == 1);
  REQUIRE(c.sizes()[1] == 2);
  REQUIRE(c.sizes()[2] == 2);

  // advanced integer index embedded in extract
  Series idx = pluss::make_series(std::vector<std::int64_t>{1, 0}, {2});
  Series d = x.extract(pluss::Slicing::SliceAll(), idx, 1);
  REQUIRE(d.dim() == 2);
  REQUIRE(d.sizes()[0] == 2);
  REQUIRE(d.sizes()[1] == 2);
  REQUIRE(d.pos(0, 0).item<double>() == 4.0);
  REQUIRE(d.pos(0, 1).item<double>() == 2.0);

  // advanced boolean index embedded in extract
  Series mask = pluss::make_series(std::vector<bool>{false, true}, {2});
  Series e = x.extract(mask, pluss::Slicing::Ellipsis());
  REQUIRE(e.dim() == 3);
  REQUIRE(e.sizes()[0] == 1);
  REQUIRE(e.sizes()[1] == 2);
  REQUIRE(e.sizes()[2] == 2);
  REQUIRE(e.pos(0, 0, 0).item<double>() == 5.0);
}

TEST_CASE("Series::extract_put_ supports writable slice semantics", "[index][extract][write]") {
  Series x = pluss::series::zeros({2, 3}, ScalarType::Float64);
  Series values = pluss::make_series(std::vector<double>{9.0, 8.0}, {2});

  x.extract_put_({1, {1, 3}}, values);
  REQUIRE(x.pos(0, 0).item<double>() == 0.0);
  REQUIRE(x.pos(1, 0).item<double>() == 0.0);
  REQUIRE(x.pos(1, 1).item<double>() == 9.0);
  REQUIRE(x.pos(1, 2).item<double>() == 8.0);

  // new-axis token does not change addressing semantics for writes
  x.extract_put_({pluss::Slicing::NewAxis(), 0, 2}, pluss::make_series(4.0));
  REQUIRE(x.pos(0, 2).item<double>() == 4.0);
}
