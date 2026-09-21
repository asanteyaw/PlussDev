#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show(std::string_view label, const pluss::Note& note) {
  std::cout << label << '\n' << pluss::note::print(note) << '\n';
}

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

} // namespace

int main() {
  std::cout << "=== argsort ===\n";
  const auto words = pluss::note::make({"b", "a", "b", ""});
  show("ascending stable", words.argsort());
  show("take order", words.take(words.argsort()));
  show("descending stable", words.argsort(true, true));
  show("unstable unique values",
       pluss::note::make({"delta", "alpha", "charlie", "bravo"})
           .argsort(false, false));
  show("Bytes order",
       pluss::note::bytes(
           std::vector<std::string>{"\xFF", "\x01", "\x7F"},
           std::array<std::int64_t, 1>{3})
           .argsort());
  show("Bool order", pluss::note::make({true, false, true, false}).argsort());
  show("Int64 order",
       pluss::note::make(std::vector<std::int64_t>{10, -2, 10, 0}).argsort());
  const auto reals = pluss::note::make(std::vector<double>{
      2.0, std::numeric_limits<double>::quiet_NaN(), -1.0,
      std::numeric_limits<double>::quiet_NaN()});
  show("Float64 ascending", reals.argsort());
  show("Float64 descending", reals.argsort(true, true));
  show("scalar", pluss::note::make("scalar").argsort());
  show("empty", pluss::note::make(std::vector<std::string>{}).argsort());
  rejected("matrix", [&] { (void)words.reshape({2, 2}).argsort(); });
  rejected("undefined argsort", [&] { (void)pluss::Note::undefined().argsort(); });

  std::cout << "\n=== repeat ===\n";
  const auto vector = pluss::note::make({"a", "b"});
  show("vector chunks", vector.repeat({2}));
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2});
  show("repeat rows", matrix.repeat({2, 1}));
  show("repeat columns", matrix.repeat({1, 2}));
  show("prepend dimension", vector.repeat({2, 2}));
  show("scalar identity", pluss::note::make("x").repeat({}));
  show("scalar matrix", pluss::note::make("x").repeat({2, 3}));
  show("zero extent", matrix.repeat({0, 2}));
  show("Bool preserved", pluss::note::make({true, false}).repeat({2}));
  show("Int64 preserved",
       pluss::note::make(std::vector<std::int64_t>{1, 2}).repeat({2}));
  const auto raw = pluss::note::bytes(
      std::vector<std::string>{"A", "B"},
      std::array<std::int64_t, 1>{2});
  show("Bytes preserved", raw.repeat({2}));
  std::cout << "undefined repeat defined=" << std::boolalpha
            << pluss::Note::undefined().repeat({2}).defined() << '\n';
  rejected("too few dimensions", [&] { (void)matrix.repeat({2}); });
  rejected("negative repeat", [&] { (void)vector.repeat({-1}); });
  rejected("rank four", [&] { (void)vector.repeat({1, 1, 1, 1}); });
  rejected("repeat overflow", [&] {
    (void)pluss::note::make("x").repeat(
        {std::numeric_limits<std::int64_t>::max(), 2});
  });

  std::cout << "\n=== repeat_interleave ===\n";
  show("vector lanes", vector.repeat_interleave(3, 0));
  show("matrix rows", matrix.repeat_interleave(2, 0));
  show("matrix columns", matrix.repeat_interleave(2, -1));
  show("scalar dim zero", pluss::note::make("x").repeat_interleave(3, 0));
  show("scalar dim minus one", pluss::note::make("x").repeat_interleave(2, -1));
  show("zero interleave", matrix.repeat_interleave(0, 1));
  show("Float64 preserved",
       pluss::note::make(std::vector<double>{1.5, 2.5})
           .repeat_interleave(2, 0));
  show("Bytes interleave", raw.repeat_interleave(2, 0));
  std::cout << "undefined interleave defined=" << std::boolalpha
            << pluss::Note::undefined().repeat_interleave(2, 0).defined() << '\n';
  rejected("negative interleave", [&] { (void)vector.repeat_interleave(-1, 0); });
  rejected("axis out of range", [&] { (void)matrix.repeat_interleave(2, 2); });
  rejected("scalar axis out of range", [&] {
    (void)pluss::note::make("x").repeat_interleave(2, 1);
  });
  rejected("interleave overflow", [&] {
    (void)vector.repeat_interleave(
        std::numeric_limits<std::int64_t>::max(), 0);
  });
}
