#include <pluss/pluss.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show_shape(const pluss::Note& value) {
  std::cout << '[';
  const auto shape = value.sizes_view();
  for (std::size_t i = 0; i < shape.size(); ++i)
    std::cout << (i ? ", " : "") << shape[i];
  std::cout << ']';
}

void show_batch(const pluss::TokenBatch& batch) {
  const auto bounds = batch.offsets().integers();
  std::cout << "tokens=" << batch.tokens() << '\n' << "offsets=[";
  for (std::size_t i = 0; i < bounds.size(); ++i)
    std::cout << (i ? ", " : "") << bounds[i];
  std::cout << "]\n";
  for (std::size_t row = 0; row + 1 < bounds.size(); ++row) {
    std::cout << "row " << row << ": [";
    for (auto i = bounds[row]; i < bounds[row + 1]; ++i)
      std::cout << (i == bounds[row] ? "" : ", ") << '"'
                << batch.tokens().at(i) << '"';
    std::cout << "]\n";
  }
}

template <class Fn>
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
  const auto absent = pluss::Note::undefined();
  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  std::cout << std::boolalpha
            << "=== undefined and empty ===\n"
            << "absent: defined=" << absent.defined()
            << " dim=" << absent.dim() << " numel=" << absent.numel()
            << " empty=" << absent.empty()
            << " scalar=" << absent.is_scalar() << '\n'
            << "empty: defined=" << empty.defined()
            << " dim=" << empty.dim() << " numel=" << empty.numel()
            << " empty=" << empty.empty() << " shape=";
  show_shape(empty);
  std::cout << '\n';
  rejected("undefined dtype", [&] { (void)absent.dtype(); });
  const auto absent_copy = absent;
  std::cout << "copied defined=" << absent_copy.defined() << '\n';

  std::cout << "\n=== debug states ===\n"
            << absent.debug() << '\n'
            << pluss::note::make(
                   std::vector<std::string>{"alpha", "line\n\"q\"", "omega"})
                   .debug(2)
            << '\n'
            << pluss::note::make(std::vector<std::int64_t>{-2, 0, 7}).debug() << '\n'
            << pluss::note::make(std::vector<bool>{true, false}).debug() << '\n'
            << pluss::note::make(std::vector<double>{-0.0, 1.25}).debug() << '\n'
            << pluss::note::make({"a", "b", "c"}).debug(0) << '\n'
            << pluss::note::make({"a", "b", "c"}).debug(-3) << '\n';
  const std::string raw{"A\0\xFF", 3};
  std::cout << pluss::note::bytes(std::vector<std::string>{raw}).debug() << '\n';

  std::cout << "\n=== pack envelope ===\n";
  const auto source = pluss::note::make(
      std::vector<std::int64_t>{-1, 0, 7, 9},
      std::array<std::int64_t, 2>{2, 2});
  const auto encoded = source.pack();
  std::cout << "magic=";
  for (std::size_t i = 0; i < 4; ++i)
    std::cout << std::to_integer<char>(encoded[i]);
  std::cout << " version=" << std::to_integer<unsigned>(encoded[4])
            << " rank=" << std::to_integer<unsigned>(encoded[6])
            << " bytes=" << encoded.size() << '\n';
  const auto restored = pluss::note::unpack(encoded);
  std::cout << "restored=" << restored << '\n';
  std::cout << "undefined bytes=" << absent.pack().size()
            << " restored-defined="
            << pluss::note::unpack(absent.pack()).defined() << '\n';
  const auto empty_encoded = empty.pack();
  const auto empty_restored = pluss::note::unpack(empty_encoded);
  std::cout << "empty bytes=" << empty_encoded.size() << " shape=";
  show_shape(empty_restored);
  std::cout << '\n';

  const auto float_source = pluss::note::make(std::vector<double>{
      -0.0, std::bit_cast<double>(UINT64_C(0x7ff8000000000000))});
  const auto float_restored = pluss::note::unpack(float_source.pack());
  std::cout << std::hex << std::showbase
            << "float bits="
            << std::bit_cast<std::uint64_t>(float_restored.reals()[0]) << ','
            << std::bit_cast<std::uint64_t>(float_restored.reals()[1])
            << std::dec << std::noshowbase << '\n';

  std::cout << "\n=== split chars ===\n";
  show_batch(pluss::note::make({"Aé🙂", "Hi"}).split_chars());

  const auto decomposed = pluss::note::make("e\u0301").split_chars();
  std::cout << "decomposed token-count=" << decomposed.tokens().numel()
            << " byte-lengths=" << decomposed.tokens().at(0).size()
            << ',' << decomposed.tokens().at(1).size() << '\n';

  const auto grid = pluss::note::make({{"ab", ""}, {"Z", "xy"}});
  const auto grid_chars = grid.split_chars();
  std::cout << "grid source-shape=";
  show_shape(grid);
  std::cout << " offsets=";
  for (const auto value : grid_chars.offsets().integers())
    std::cout << value << ' ';
  std::cout << '\n';

  const auto absent_chars = absent.split_chars();
  std::cout << "undefined batch=" << absent_chars.defined()
            << " tokens-defined=" << absent_chars.tokens().defined()
            << " offset=" << absent_chars.offsets().integers()[0] << '\n';
  rejected("Bytes split", [&] {
    (void)pluss::note::bytes(std::vector<std::string>{"abc"}).split_chars();
  });
  rejected("Int64 split", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{1, 2}).split_chars();
  });
}
