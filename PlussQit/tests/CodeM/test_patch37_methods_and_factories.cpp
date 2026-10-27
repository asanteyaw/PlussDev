#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("factory Config kv list works", "[factories]") {
  auto x = pluss::series::zeros({2,3}, pluss::config({{"dtype","float16"}}));
  REQUIRE(x.scalar_type() == pluss::ScalarType::Float16);

  auto y = pluss::series::full({2,3}, 1.0, pluss::config({{"dtype","bfloat16"}}));
  REQUIRE(y.scalar_type() == pluss::ScalarType::BFloat16);
}

TEST_CASE("pile/unite methods work", "[methods]") {
  auto a = pluss::series::ones({2,2}, pluss::ScalarType::Float32);
  auto b = pluss::series::zeros({2,2}, pluss::ScalarType::Float32);
  auto s = a.pile({b}, 0);
  REQUIRE(s.sizes()[0] == 2); // stacked along new dim: [2,2,2]
  REQUIRE(s.sizes()[1] == 2);
  REQUIRE(s.sizes()[2] == 2);

  auto c = a.unite({b}, 0);
  REQUIRE(c.sizes()[0] == 4);
  REQUIRE(c.sizes()[1] == 2);
}

TEST_CASE("sign_list supports temporary and lvalue usage", "[methods][list]") {
  auto a = pluss::series::ones({2,2}, ScalarType::Float32);
  auto b = pluss::series::zeros({2,2}, ScalarType::Float32);

  auto stacked_temp = pluss::series::pile(pluss::sign_list(a, b), 0);
  REQUIRE(stacked_temp.sizes()[0] == 2);
  REQUIRE(stacked_temp.sizes()[1] == 2);
  REQUIRE(stacked_temp.sizes()[2] == 2);

  auto list = pluss::sign_list(a, b);
  REQUIRE(list.get(0).sizes()[0] == 2);
  REQUIRE(list.get(1).sizes()[0] == 2);
  REQUIRE_THROWS_AS(list.get(2), pluss::Error);
  REQUIRE_THROWS_AS(list.get(-1), pluss::Error);
  REQUIRE_THROWS_AS((void)list[2], pluss::Error);

  auto extended_one = list.extend(a);
  REQUIRE(list.size() == 2);
  REQUIRE(extended_one.size() == 3);
  REQUIRE(extended_one.get(2).sizes()[0] == 2);

  auto extended_two = list.extend({a, b});
  REQUIRE(extended_two.size() == 4);
  REQUIRE(extended_two.get(3).sizes()[0] == 2);

  auto united_lvalue = pluss::series::unite(list, 0);
  REQUIRE(united_lvalue.sizes()[0] == 4);
  REQUIRE(united_lvalue.sizes()[1] == 2);

  auto owned = pluss::msharp::Lister::Series{a, b};
  auto stacked_owned = pluss::series::pile(owned, 0);
  REQUIRE(stacked_owned.sizes()[0] == 2);
  REQUIRE(stacked_owned.sizes()[1] == 2);
  REQUIRE(stacked_owned.sizes()[2] == 2);

  auto united_owned = pluss::series::unite(owned, 0);
  REQUIRE(united_owned.sizes()[0] == 4);
  REQUIRE(united_owned.sizes()[1] == 2);
}

TEST_CASE("pair reductions return values and indices in one call", "[methods][list]") {
  auto x = pluss::make_series({1.0, 5.0, 2.0, 3.0, 4.0, 6.0}).reshape({2, 3});
  auto out = x.max_pair(1, false);
  REQUIRE(out.first.host<double>() == std::vector<double>{5.0, 6.0});
  REQUIRE(out.second.host<std::int64_t>() == std::vector<std::int64_t>{1, 2});

  auto idx = x.argmax(1);
  REQUIRE(idx.host<std::int64_t>() == std::vector<std::int64_t>{1, 2});

  auto min_out = x.min_pair(1, false);
  REQUIRE(min_out.first.host<double>() == std::vector<double>{1.0, 3.0});
  REQUIRE(min_out.second.host<std::int64_t>() == std::vector<std::int64_t>{0, 0});

  auto min_idx = x.argmin(1);
  REQUIRE(min_idx.host<std::int64_t>() == std::vector<std::int64_t>{0, 0});
}

TEST_CASE("sort_pair and topk_pair return values+indices", "[methods][list]") {
  auto x = pluss::make_series({1.0, 5.0, 2.0, 3.0, 4.0, 6.0}).reshape({2, 3});

  auto sort_out = x.sort_pair(1, /*descending=*/true, /*stable=*/false);
  REQUIRE(sort_out.first.host<double>() == std::vector<double>{5.0, 2.0, 1.0, 6.0, 4.0, 3.0});
  REQUIRE(sort_out.second.host<std::int64_t>() == std::vector<std::int64_t>{1, 2, 0, 2, 1, 0});

  auto topk_out = x.topk_pair(/*k=*/2, /*dim=*/1, /*largest=*/true, /*sorted=*/true);
  REQUIRE(topk_out.first.host<double>() == std::vector<double>{5.0, 2.0, 6.0, 4.0});
  REQUIRE(topk_out.second.host<std::int64_t>() == std::vector<std::int64_t>{1, 2, 2, 1});

  auto unordered = x.topk_pair(/*k=*/2, /*dim=*/1, /*largest=*/false, /*sorted=*/false);
  REQUIRE(unordered.first.sizes() == std::vector<std::int64_t>{2, 2});
  const auto unordered_values = unordered.first.host<double>();
  const auto unordered_indices = unordered.second.host<std::int64_t>();
  const auto input_values = x.host<double>();
  for (std::int64_t row = 0; row < 2; ++row) {
    for (std::int64_t col = 0; col < 2; ++col) {
      const auto output_offset = row * 2 + col;
      const auto index = unordered_indices[static_cast<std::size_t>(output_offset)];
      REQUIRE(unordered_values[static_cast<std::size_t>(output_offset)] ==
              input_values[static_cast<std::size_t>(row * 3 + index)]);
    }
  }
  REQUIRE_THROWS_WITH(x.topk_pair(4, 1),
                      Catch::Matchers::ContainsSubstring("k must be <= size(dim)"));
}

TEST_CASE("extrema pairs keep values, indices, and reverse routing aligned", "[methods][reduction][autodiff]") {
  auto x = pluss::make_series({{4.0, 4.0, 1.0}, {2.0, -3.0, -3.0}});
  x.tracks_slope_(true);

  auto maxima = x.max_pair(-1, true);
  REQUIRE(maxima.first.sizes() == std::vector<std::int64_t>{2, 1});
  REQUIRE(maxima.second.sizes() == std::vector<std::int64_t>{2, 1});
  REQUIRE(maxima.first.host<double>() == std::vector<double>{4.0, 2.0});
  REQUIRE(maxima.second.host<std::int64_t>() == std::vector<std::int64_t>{0, 0});

  maxima.first.sum().reverse();
  REQUIRE(x.sensitivity().host<double>() ==
          std::vector<double>{1.0, 0.0, 0.0, 1.0, 0.0, 0.0});

  auto minima = x.min_pair(1, false);
  REQUIRE(minima.first.sizes() == std::vector<std::int64_t>{2});
  REQUIRE(minima.first.host<double>() == std::vector<double>{1.0, -3.0});
  REQUIRE(minima.second.host<std::int64_t>() == std::vector<std::int64_t>{2, 1});
}

TEST_CASE("extrema pairs validate scalar and empty-axis contracts", "[methods][reduction][errors]") {
  const auto scalar = pluss::make_series(7.0);
  const auto scalar_pair = scalar.max_pair(0);
  REQUIRE(scalar_pair.first.sizes().empty());
  REQUIRE(scalar_pair.first.item<double>() == 7.0);
  REQUIRE(scalar_pair.second.item<std::int64_t>() == 0);

  REQUIRE_THROWS_WITH(scalar.max_pair(1), Catch::Matchers::ContainsSubstring("dim out of range"));

  const auto empty = pluss::Series::empty({2, 0}, pluss::ScalarType::Float64);
  REQUIRE_THROWS_WITH(empty.max_pair(1),
                      Catch::Matchers::ContainsSubstring("cannot reduce an empty dimension"));
  REQUIRE_THROWS_WITH(empty.min_pair(-1),
                      Catch::Matchers::ContainsSubstring("cannot reduce an empty dimension"));
}
