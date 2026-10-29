#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

using pluss::Series;
using pluss::ScalarType;
using pluss::make_series;

namespace {
using Dims = std::vector<std::int64_t>;

template <class T>
std::vector<T> flat(const Series& s) {
  return s.contiguous().reshape(Dims{s.numel()}).host<T>();
}

// Reference permute: out[i0..] = src[ idx scattered by perm ], from the
// row-major source data alone (no Series ops involved).
template <class T>
std::vector<T> ref_permute(const std::vector<T>& src, const Dims& shape, const Dims& perm) {
  const std::size_t nd = shape.size();
  Dims sstr(nd, 1);
  for (std::size_t i = nd; i-- > 1;) sstr[i - 1] = sstr[i] * shape[i];
  Dims oshape(nd);
  for (std::size_t i = 0; i < nd; ++i) oshape[i] = shape[static_cast<std::size_t>(perm[i])];
  std::vector<T> out(src.size());
  Dims idx(nd, 0);
  for (std::size_t lin = 0; lin < out.size(); ++lin) {
    std::int64_t so = 0;
    for (std::size_t i = 0; i < nd; ++i) so += idx[i] * sstr[static_cast<std::size_t>(perm[i])];
    out[lin] = src[static_cast<std::size_t>(so)];
    for (std::size_t i = nd; i-- > 0;) {
      if (++idx[i] < oshape[i]) break;
      idx[i] = 0;
    }
  }
  return out;
}

template <class T>
void check_perm(const Dims& shape, const Dims& perm, ScalarType st) {
  std::int64_t n = 1;
  for (auto d : shape) n *= d;
  std::vector<T> data(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) data[static_cast<std::size_t>(i)] = static_cast<T>((i * 7 + 3) % 113);
  const Series x = make_series(data, shape).to(st);
  const Series y = x.permute(perm).contiguous();
  REQUIRE(y.is_contiguous());
  REQUIRE(flat<T>(y) == ref_permute(data, shape, perm));
}
}  // namespace

TEST_CASE("permute().contiguous() copies correctly for every layout class", "[strided-copy]") {
  // small, tiled (transpose-like, both axes >= 8), parallel-sized, odd tile edges, size-1 axes
  const std::vector<std::pair<Dims, Dims>> cases = {
      {{5, 7}, {1, 0}},          {{40, 33}, {1, 0}},        {{128, 130}, {1, 0}},
      {{3, 4, 5}, {2, 0, 1}},    {{4, 40, 33}, {0, 2, 1}},  {{4, 40, 33}, {2, 1, 0}},
      {{128, 64, 32}, {1, 0, 2}}, {{64, 128, 32}, {2, 0, 1}}, {{16, 1, 70}, {2, 1, 0}},
      {{2, 3, 4, 5}, {3, 1, 0, 2}}, {{9, 9, 9, 9}, {3, 2, 1, 0}}, {{1, 1, 1}, {2, 0, 1}},
      {{600, 500}, {1, 0}},      {{50, 60, 70}, {1, 2, 0}},
  };
  for (const auto& [shape, perm] : cases) {
    INFO("shape rank " << shape.size());
    check_perm<double>(shape, perm, ScalarType::Float64);
    check_perm<float>(shape, perm, ScalarType::Float32);
    check_perm<std::int64_t>(shape, perm, ScalarType::Int64);
    check_perm<std::int32_t>(shape, perm, ScalarType::Int32);
    check_perm<std::int8_t>(shape, perm, ScalarType::Int8);
  }
}

TEST_CASE("sliced and broadcast views copy correctly", "[strided-copy]") {
  std::vector<double> d(6 * 40);
  std::iota(d.begin(), d.end(), 0.0);
  const Series x = make_series(d, {6, 40});
  const Series v = x.slice(1, 3, 35).slice(0, 1, 5).transpose(0, 1).contiguous();  // [32, 4] strided
  REQUIRE(v.sizes()[0] == 32);
  const auto h = flat<double>(v);
  for (int i = 0; i < 32; ++i)
    for (int j = 0; j < 4; ++j) REQUIRE(h[i * 4 + j] == d[(1 + j) * 40 + 3 + i]);
  const Series e = make_series(std::vector<double>{1, 2, 3}, {3, 1}).expand(Dims{3, 5}).contiguous();
  REQUIRE(flat<double>(e) == std::vector<double>{1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3});
}

TEST_CASE("integer matmul (int8/uint8/int16/int32/int64), wraparound and batching", "[matmul][int]") {
  for (auto st : {ScalarType::UInt8, ScalarType::Int16, ScalarType::Int32, ScalarType::Int64}) {
    const Series a = make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3}).to(st);
    const Series b = make_series(std::vector<double>{7, 8, 9, 10, 11, 12}, {3, 2}).to(st);
    const Series c = a.matmul(b);
    REQUIRE(c.scalar_type() == st);
    REQUIRE(flat<double>(c.to(ScalarType::Float64)) == std::vector<double>{58, 64, 139, 154});
  }
  // int64 beyond 2^53 stays exact, and overflow wraps like two's complement.
  const std::int64_t big = (std::int64_t{1} << 62) + 1;
  const Series a = make_series(std::vector<std::int64_t>{big, 1}, {1, 2});
  const Series b = make_series(std::vector<std::int64_t>{1, big}, {2, 1});
  const auto r = flat<std::int64_t>(a.matmul(b))[0];
  REQUIRE(r == static_cast<std::int64_t>(static_cast<std::uint64_t>(big) + static_cast<std::uint64_t>(big)));
  // einsum/tensordot on integer operands
  const Series m = make_series(std::vector<std::int64_t>{1, 2, 3, 4}, {2, 2});
  REQUIRE(flat<std::int64_t>(Series::einsum("ij,jk->ik", {m, m})) == std::vector<std::int64_t>{7, 10, 15, 22});
  REQUIRE(flat<std::int64_t>(m.tensordot(m, 1)) == std::vector<std::int64_t>{7, 10, 15, 22});
  // batched
  const Series bt = make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6, 7, 8}, {2, 2, 2});
  REQUIRE(flat<std::int64_t>(bt.bmm(bt)) == std::vector<std::int64_t>{7, 10, 15, 22, 67, 78, 91, 106});
}

TEST_CASE("trace gradient on a tall matrix touches only the diagonal", "[trace]") {
  // libtorch's trace backward writes every (cols+1)-th flat slot, which for
  // rows > cols lands on an off-diagonal entry; the exact gradient is the
  // identity pattern on the min(rows, cols) diagonal and zero elsewhere.
  Series x = make_series(std::vector<double>(15, 1.0), {5, 3});
  x.tracks_slope_(true);
  x.trace().reverse();
  const auto g = flat<double>(x.sensitivity());
  for (int r = 0; r < 5; ++r)
    for (int c = 0; c < 3; ++c) REQUIRE(g[r * 3 + c] == (r == c ? 1.0 : 0.0));
}
