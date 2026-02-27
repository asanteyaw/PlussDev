#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <cmath>

TEST_CASE("softmax/log_softmax/logsumexp run", "[softmax]") {
  auto x = pluss::make_series(std::vector<float>{0,1,2,  1,2,3}, {2,3});
  auto y = x.softmax(1);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.sizes()[1] == 3);

  auto l = x.log_softmax(1);
  REQUIRE(l.sizes()[0] == 2);

  auto s = x.logsumexp(1);
  REQUIRE(s.sizes().size() == 1);
  REQUIRE(s.sizes()[0] == 2);

  auto s2 = x.logsumexp(1, true);
  REQUIRE(s2.sizes().size() == 2);
  REQUIRE(s2.sizes()[0] == 2);
  REQUIRE(s2.sizes()[1] == 1);
}
