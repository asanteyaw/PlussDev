#include <pluss/pluss.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show_offsets(const pluss::TokenBatch& batch) {
  std::cout << '[';
  const auto values = batch.offsets().integers();
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << (i == 0 ? "" : ", ") << values[i];
  }
  std::cout << "]\n";
}

void show_rows(const pluss::TokenBatch& batch) {
  const auto bounds = batch.offsets().integers();
  for (std::size_t row = 0; row + 1 < bounds.size(); ++row) {
    std::cout << "row " << row << ": [";
    for (std::int64_t i = bounds[row]; i < bounds[row + 1]; ++i) {
      std::cout << (i == bounds[row] ? "" : ", ") << '"'
                << batch.tokens().at(i) << '"';
    }
    std::cout << "]\n";
  }
}

void show_shape(const pluss::Note& value) {
  std::cout << '[';
  const auto shape = value.sizes_view();
  for (std::size_t i = 0; i < shape.size(); ++i) {
    std::cout << (i == 0 ? "" : ", ") << shape[i];
  }
  std::cout << ']';
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

void show_round_trip(std::string_view label, const pluss::Note& source) {
  const auto encoded = source.pack();
  const auto restored = pluss::note::unpack(encoded);
  std::cout << label << ": defined=" << std::boolalpha << restored.defined();
  if (restored.defined()) {
    std::cout << " dtype=" << pluss::dtype_name(restored.dtype())
              << " shape=";
    show_shape(restored);
    std::cout << " bytes=" << encoded.size();
  }
  std::cout << '\n';
}

} // namespace

int main() {
  std::cout << "=== TokenBatch states ===\n";
  const pluss::TokenBatch absent;
  std::cout << "default: defined=" << std::boolalpha << absent.defined()
            << " empty=" << absent.empty() << '\n';
  rejected("default tokens", [&] { (void)absent.tokens(); });
  rejected("default offsets", [&] { (void)absent.offsets(); });

  const auto batch = pluss::note::make({"red green", "", "blue"}).nlp().tokens();
  std::cout << "populated: defined=" << batch.defined()
            << " empty=" << batch.empty() << '\n';
  std::cout << "tokens dtype=" << pluss::dtype_name(batch.tokens().dtype())
            << " shape=";
  show_shape(batch.tokens());
  std::cout << " values=[";
  for (std::int64_t i = 0; i < batch.tokens().numel(); ++i) {
    std::cout << (i == 0 ? "" : ", ") << '"' << batch.tokens().at(i) << '"';
  }
  std::cout << "]\n";
  std::cout << "offsets dtype=" << pluss::dtype_name(batch.offsets().dtype())
            << " shape=";
  show_shape(batch.offsets());
  std::cout << " values=";
  show_offsets(batch);
  show_rows(batch);

  const auto empty = pluss::note::make({"", "!!!"}).nlp().tokens();
  std::cout << "all-empty: defined=" << empty.defined()
            << " empty=" << empty.empty()
            << " tokens-defined=" << empty.tokens().defined() << " offsets=";
  show_offsets(empty);

  const auto propagated = pluss::Note::undefined().nlp().tokens();
  std::cout << "propagated: defined=" << propagated.defined()
            << " empty=" << propagated.empty()
            << " tokens-defined=" << propagated.tokens().defined()
            << " offsets=";
  show_offsets(propagated);

  auto retained = batch.tokens();
  auto copied_batch = batch;
  std::cout << "shared handles: retained-first=" << retained.at(0)
            << " copied-last=" << copied_batch.tokens().at(2) << '\n';

  std::cout << "\n=== unpack round trips ===\n";
  show_round_trip("undefined", pluss::Note::undefined());
  show_round_trip("text scalar", pluss::note::make("alpha"));
  const std::string raw{"a\0b", 3};
  show_round_trip("bytes matrix",
                  pluss::note::bytes(std::vector<std::string_view>{raw, "", "z", "q"},
                                     std::array<std::int64_t, 2>{2, 2}));
  show_round_trip("bool matrix",
                  pluss::note::make(std::vector<bool>{true, false, false, true},
                                    std::array<std::int64_t, 2>{2, 2}));
  show_round_trip("int matrix",
                  pluss::note::make(std::vector<std::int64_t>{-1, 0, 7, 9},
                                    std::array<std::int64_t, 2>{2, 2}));
  show_round_trip("float matrix",
                  pluss::note::make(
                      std::vector<double>{-0.0, 1.25,
                                          std::numeric_limits<double>::infinity(),
                                          std::numeric_limits<double>::quiet_NaN()},
                      std::array<std::int64_t, 2>{2, 2}));
  show_round_trip("empty matrix",
                  pluss::note::make(std::vector<bool>{},
                                    std::array<std::int64_t, 2>{0, 5}));

  const auto text = pluss::note::make(std::vector<std::string>{raw, "tail"});
  const auto restored_text = pluss::note::unpack(text.pack());
  std::cout << "embedded zero: length=" << restored_text.at(0).size()
            << " middle=" << static_cast<int>(restored_text.at(0)[1])
            << " second=" << restored_text.at(1) << '\n';

  auto owned = pluss::Note::undefined();
  {
    const auto temporary = pluss::note::make("alpha").pack();
    owned = pluss::note::unpack(temporary);
  }
  std::cout << "owned after buffer: " << owned.scalar() << '\n';

  const auto floats = pluss::note::make(
      std::vector<double>{-0.0, std::numeric_limits<double>::quiet_NaN()});
  const auto restored_floats = pluss::note::unpack(floats.pack());
  std::cout << "float bits: negzero=" << std::hex << std::showbase
            << std::bit_cast<std::uint64_t>(restored_floats.reals()[0])
            << " nan=" << std::bit_cast<std::uint64_t>(restored_floats.reals()[1])
            << std::dec << std::noshowbase << '\n';

  const auto envelope = pluss::note::make({true, false}).pack();
  std::cout << "header: magic=";
  for (std::size_t i = 0; i < 4; ++i)
    std::cout << static_cast<char>(envelope[i]);
  std::cout << " version=" << static_cast<unsigned>(envelope[4])
            << " rank=" << static_cast<unsigned>(envelope[6])
            << " bytes=" << envelope.size() << '\n';

  auto bad_magic = envelope;
  bad_magic[0] = std::byte{0};
  rejected("bad magic", [&] { (void)pluss::note::unpack(bad_magic); });
  auto bad_version = envelope;
  bad_version[4] = std::byte{99};
  rejected("bad version", [&] { (void)pluss::note::unpack(bad_version); });
  auto bad_dtype = envelope;
  bad_dtype[5] = std::byte{99};
  rejected("bad dtype", [&] { (void)pluss::note::unpack(bad_dtype); });
  auto bad_bool = envelope;
  bad_bool.back() = std::byte{2};
  rejected("noncanonical bool", [&] { (void)pluss::note::unpack(bad_bool); });
  auto trailing = envelope;
  trailing.push_back(std::byte{0});
  rejected("trailing byte", [&] { (void)pluss::note::unpack(trailing); });
  rejected("truncated", [&] {
    (void)pluss::note::unpack(
        std::span<const std::byte>(envelope.data(), envelope.size() - 1));
  });
}
