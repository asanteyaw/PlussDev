#include <catch2/catch_test_macros.hpp>

#include <pluss/tils/aux/transmit.h>

#include <cstddef>
#include <vector>

namespace {

struct NeedsIndex {
  int operator()(int value, std::size_t index) const {
    return value + static_cast<int>(index);
  }
};

template <class F, class T>
concept ScalarTransmittable = requires(F fn, T value) {
  pluss::tils::transmit(fn, value);
};

static_assert(!ScalarTransmittable<NeedsIndex, int>);

} // namespace

TEST_CASE("scalar transmit does not invent an element index", "[aux][transmit]") {
  const auto doubled = pluss::tils::transmit([](int value) { return value * 2; }, 7);
  REQUIRE(doubled == 14);
}

TEST_CASE("range transmit supplies an index only to indexed range callables", "[aux][transmit]") {
  const std::vector<int> values{10, 20, 30};

  const auto plain = pluss::tils::transmit([](int value) { return value + 1; }, values);
  REQUIRE(plain == std::vector<int>{11, 21, 31});

  const auto indexed = pluss::tils::transmit(NeedsIndex{}, values);
  REQUIRE(indexed == std::vector<int>{10, 21, 32});
}
