#include <pluss/pluss.h>

#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Duration& value) {
  std::cout << "\n== " << label << " ==\n"
            << pluss::duration::print(value) << "\n";
}

void show_host(std::string_view label, const pluss::Duration& value) {
  std::cout << "\n== " << label << " ==\n[";
  const auto host = value.host();
  for (std::size_t i = 0; i < host.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << host[i];
  }
  std::cout << "]\n";
}

template <typename F>
void show_error(std::string_view label, F&& call) {
  std::cout << "\n== " << label << " ==\n";
  try {
    call();
    std::cout << "no error\n";
  } catch (const std::exception& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
}

#define SHOW_UNIT(name)                                                        \
  show(#name "_scalar", pluss::duration::name(-2));                          \
  show(#name "_vector", pluss::duration::name({0, 1, 2}));                   \
  show(#name "_nested", pluss::duration::name({{1, 2}, {3, 4}}));            \
  show(#name "_flat_shape", pluss::duration::name({1, 2, 3, 4}, {2, 2}))

} // namespace

int main() {
  SHOW_UNIT(nanos);
  SHOW_UNIT(microseconds);
  SHOW_UNIT(milliseconds);
  SHOW_UNIT(seconds);
  SHOW_UNIT(minutes);
  SHOW_UNIT(hours);
  SHOW_UNIT(days);
  SHOW_UNIT(weeks);

#undef SHOW_UNIT

  show_host("nanos_exact_int64",
            pluss::duration::nanos(9'007'199'254'740'993LL));
  show("nanos_nat", pluss::duration::nanos(pluss::Duration::kNaT));
  show_error("unit_scale_overflow", [] {
    (void)pluss::duration::weeks((std::numeric_limits<std::int64_t>::max)());
  });

  show("make_runtime_unit",
       pluss::duration::make({1, 2, 3, 4}, {2, 2},
                             pluss::duration::Unit::Minutes));
  const auto canonical = pluss::duration::make(
      {1, 2, 3}, pluss::duration::Unit::Hours);
  const auto shortcut = pluss::make_duration(
      {1, 2, 3}, pluss::duration::Unit::Hours);
  std::cout << "\n== make_duration_equivalent ==\n"
            << std::boolalpha << (canonical.host() == shortcut.host()) << "\n";
  show_error("make_invalid_unit", [] {
    (void)pluss::duration::make(
        1, static_cast<pluss::duration::Unit>(999));
  });

  show("range_ascending",
       pluss::duration::range(pluss::duration::seconds(0),
                              pluss::duration::seconds(5),
                              pluss::duration::seconds(2)));
  show("range_descending",
       pluss::duration::range(pluss::duration::seconds(5),
                              pluss::duration::seconds(0),
                              pluss::duration::seconds(-2)));
  show("range_wrong_direction",
       pluss::duration::range(pluss::duration::seconds(5),
                              pluss::duration::seconds(0),
                              pluss::duration::seconds(2)));
  const auto hi = (std::numeric_limits<std::int64_t>::max)();
  show_host("range_int64_boundary",
            pluss::duration::range(pluss::duration::nanos(hi - 1),
                                   pluss::duration::nanos(hi),
                                   pluss::duration::nanos(1)));
  show_error("range_zero_step", [] {
    (void)pluss::duration::range(pluss::duration::seconds(0),
                                 pluss::duration::seconds(1),
                                 pluss::duration::seconds(0));
  });

  show("zeros_scalar", pluss::duration::zeros({}));
  show("zeros_matrix", pluss::duration::zeros({2, 3}));
  show("zeros_empty", pluss::duration::zeros({2, 0, 3}));
  const auto like = pluss::duration::seconds({1, 2, 3, 4}, {2, 2});
  show("zeros_like_matrix", pluss::duration::zeros_like(like));

  show("full_negative", pluss::duration::full(
      {2, 2}, pluss::duration::seconds(-3)));
  show("full_nat", pluss::duration::full(
      {3}, pluss::duration::nanos(pluss::Duration::kNaT)));
  show("full_empty", pluss::duration::full(
      {2, 0}, pluss::duration::seconds(7)));
  show_error("full_non_scalar", [] {
    (void)pluss::duration::full(
        {2}, pluss::duration::seconds({1, 2}));
  });
  show("full_like", pluss::duration::full_like(
      like, pluss::duration::nanos(7)));

  show_host("random_seed_42", pluss::duration::random(
      {2, 3}, pluss::duration::nanos(10), pluss::duration::nanos(20), 42));
  const auto random_a = pluss::duration::random(
      {4}, pluss::duration::seconds(-1), pluss::duration::seconds(1), 91);
  const auto random_b = pluss::duration::random(
      {4}, pluss::duration::seconds(-1), pluss::duration::seconds(1), 91);
  std::cout << "\n== random_repeatable ==\n"
            << std::boolalpha << (random_a.host() == random_b.host()) << "\n";
  show_error("random_reversed_bounds", [] {
    (void)pluss::duration::random(
        {2}, pluss::duration::seconds(2), pluss::duration::seconds(1), 42);
  });
  show_error("random_nat_bound", [] {
    (void)pluss::duration::random(
        {2}, pluss::duration::nanos(pluss::Duration::kNaT),
        pluss::duration::seconds(1), 42);
  });
  show_error("shape_negative_extent", [] {
    (void)pluss::duration::zeros({2, -1});
  });
  show_error("shape_rank_four", [] {
    (void)pluss::duration::zeros({1, 1, 1, 1});
  });

  const auto printable = pluss::duration::nanos({0, -1});
  std::cout << "\n== print_string ==\n"
            << pluss::duration::print(printable) << "\n";
  std::ostringstream stream;
  pluss::duration::print(stream, printable);
  std::ostringstream insertion;
  insertion << printable;
  std::cout << "\n== print_overloads_equal ==\n"
            << std::boolalpha
            << (stream.str() == pluss::duration::print(printable)) << ' '
            << (insertion.str() == stream.str()) << "\n";
}
