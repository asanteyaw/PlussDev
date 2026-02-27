#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include "CodeM/compat/Invoke.h"

#include <cstdint>
#include <vector>

TEST_CASE("invoke routes split/chunk/tensor_split first-slice projection", "[invoke][surface]") {
  auto x = pluss::make_series(std::vector<std::int64_t>{0,1,2,3,4,5,6,7,8,9}, {10});

  auto s = pluss::codem::_compat::invoke("split", x, 3LL, 0LL);
  REQUIRE(s.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1,2});

  auto c = pluss::codem::_compat::invoke("chunk", x, 4LL, 0LL);
  REQUIRE(c.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1,2});

  auto ts_sections = pluss::codem::_compat::invoke("tensor_split", x, 3LL, 0LL);
  REQUIRE(ts_sections.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1,2,3});

  auto ts_bounds = pluss::codem::_compat::invoke("tensor_split", x, std::vector<std::int64_t>{4,7}, 0LL);
  REQUIRE(ts_bounds.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1,2,3});

  auto us = pluss::codem::_compat::invoke("unsafe_split", x, 2LL, 0LL);
  REQUIRE(us.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1});

  auto uc = pluss::codem::_compat::invoke("unsafe_chunk", x, 6LL, 0LL);
  REQUIRE(uc.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1});
}

#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#include <torch/torch.h>
#include <cstring>

static std::vector<std::int64_t> torch_flat_i64(const torch::Tensor& t) {
  auto c = t.contiguous().view({-1});
  std::vector<std::int64_t> out(static_cast<std::size_t>(c.numel()));
  std::memcpy(out.data(), c.data_ptr<std::int64_t>(), out.size() * sizeof(std::int64_t));
  return out;
}

TEST_CASE("invoke routed ops match torch cpu for routed method surface", "[invoke][torch][surface]") {
  auto xt = torch::tensor({3,1,2,0,4}, torch::TensorOptions().dtype(torch::kInt64));
  auto x = pluss::codem::from_torch(xt);

  auto sort_vals = pluss::codem::_compat::invoke("sort", x, 0LL, false, false);
  auto sort_ref = std::get<0>(torch::sort(xt, 0, false, false));
  REQUIRE(sort_vals.to_vec<std::int64_t>() == torch_flat_i64(sort_ref));

  auto topk_vals = pluss::codem::_compat::invoke("topk", x, 3LL, 0LL, true, true);
  auto topk_ref = std::get<0>(torch::topk(xt, 3, 0, true, true));
  REQUIRE(topk_vals.to_vec<std::int64_t>() == torch_flat_i64(topk_ref));

  auto kth_vals = pluss::codem::_compat::invoke("kthvalue", x, 2LL, 0LL, false);
  auto kth_ref = std::get<0>(torch::kthvalue(xt, 2, 0, false));
  REQUIRE(kth_vals.to_vec<std::int64_t>() == torch_flat_i64(kth_ref));

  auto mt = torch::tensor({{1,2,3},{4,5,6},{7,8,9}}, torch::TensorOptions().dtype(torch::kInt64));
  auto m = pluss::codem::from_torch(mt);

  auto tril_vals = pluss::codem::_compat::invoke("tril", m, -1LL);
  auto tril_ref = torch::tril(mt, -1);
  REQUIRE(tril_vals.to_vec<std::int64_t>() == torch_flat_i64(tril_ref));

  auto triu_vals = pluss::codem::_compat::invoke("triu", m, 1LL);
  auto triu_ref = torch::triu(mt, 1);
  REQUIRE(triu_vals.to_vec<std::int64_t>() == torch_flat_i64(triu_ref));

  auto diag_extract_vals = pluss::codem::_compat::invoke("diag", m, -1LL);
  auto diag_extract_ref = torch::diag(mt, -1);
  REQUIRE(diag_extract_vals.to_vec<std::int64_t>() == torch_flat_i64(diag_extract_ref));

  auto vt = torch::tensor({9,8,7}, torch::TensorOptions().dtype(torch::kInt64));
  auto v = pluss::codem::from_torch(vt);
  auto diag_embed_vals = pluss::codem::_compat::invoke("diag", v, 1LL);
  auto diag_embed_ref = torch::diag(vt, 1);
  REQUIRE(diag_embed_vals.to_vec<std::int64_t>() == torch_flat_i64(diag_embed_ref));

  auto diagonal_vals = pluss::codem::_compat::invoke("diagonal", m, 0LL, 0LL, 1LL);
  auto diagonal_ref = torch::diagonal(mt, 0, 0, 1);
  REQUIRE(diagonal_vals.to_vec<std::int64_t>() == torch_flat_i64(diagonal_ref));

  auto trace_vals = pluss::codem::_compat::invoke("trace", m);
  auto trace_ref = torch::trace(mt);
  REQUIRE(trace_vals.to_vec<std::int64_t>() == torch_flat_i64(trace_ref));

  auto idx_t = torch::tensor({{2,1,0},{0,1,2},{1,1,1}}, torch::TensorOptions().dtype(torch::kInt64));
  auto idx = pluss::codem::from_torch(idx_t);
  auto take_vals = pluss::codem::_compat::invoke("take_along_dim", m, idx, 1LL);
  auto take_ref = torch::take_along_dim(mt, idx_t, 1);
  REQUIRE(take_vals.to_vec<std::int64_t>() == torch_flat_i64(take_ref));
}
#endif
