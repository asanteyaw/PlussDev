#include <pluss/pluss.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

template <typename Fn>
void rejected(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

void print_flat(const std::vector<std::string_view>& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i)
    std::cout << (i ? ", " : "") << '"' << values[i] << '"';
  std::cout << ']';
}

void print_matrix(const std::vector<pluss::Note>& values) {
  std::cout << '[';
  for (std::size_t r = 0; r < values.size(); ++r) {
    if (r) std::cout << ", ";
    print_flat(values[r].vector());
  }
  std::cout << ']';
}

void print_shape(std::span<const std::int64_t> shape) {
  std::cout << '[';
  for (std::size_t i = 0; i < shape.size(); ++i)
    std::cout << (i ? ", " : "") << shape[i];
  std::cout << ']';
}

} // namespace

int main() {
  const auto scalar = pluss::note::make("alpha");
  const auto singleton = pluss::note::make({"beta"});
  const auto one_matrix = pluss::note::make(
      std::vector<std::string>{"gamma"}, std::array<std::int64_t, 2>{1, 1});
  const auto one_tensor = pluss::note::make(
      std::vector<std::string>{"delta"}, std::array<std::int64_t, 3>{1, 1, 1});
  std::cout << "=== scalar cardinality ===\n"
            << "rank0=" << scalar.scalar() << '\n'
            << "rank1=" << singleton.scalar() << '\n'
            << "rank2=" << one_matrix.scalar() << '\n'
            << "rank3=" << one_tensor.scalar() << '\n'
            << "empty text bytes=" << pluss::note::make("").scalar().size() << '\n';
  rejected("scalar two cells", [&] { (void)pluss::note::make({"a", "b"}).scalar(); });
  rejected("scalar undefined", [&] { (void)pluss::Note::undefined().scalar(); });

  const auto grid = pluss::note::make(
      std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
      std::array<std::int64_t, 2>{2, 3});
  std::cout << "\n=== vector flatten ===\n";
  std::cout << "scalar=";
  print_flat(scalar.vector());
  std::cout << "\ngrid=";
  print_flat(grid.vector());
  std::cout << '\n';
  const auto empty_tensor = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 0, 4});
  std::cout << "empty shape=";
  print_shape(empty_tensor.sizes_view());
  std::cout << " vector size=" << empty_tensor.vector().size() << '\n';
  rejected("vector undefined", [&] { (void)pluss::Note::undefined().vector(); });

  std::cout << "\n=== matrix promotion ===\nscalar=";
  print_matrix(scalar.matrix());
  std::cout << "\nvector=";
  print_matrix(pluss::note::make({"a", "b"}).matrix());
  std::cout << "\ngrid=";
  print_matrix(grid.matrix());
  std::cout << '\n';
  const auto empty_matrix = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  const auto empty_rows = empty_matrix.matrix();
  std::cout << "empty rows=" << empty_rows.size()
            << " widths=" << empty_rows[0].numel() << ',' << empty_rows[1].numel() << '\n';
  const auto empty_vector = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 1>{0}).matrix();
  std::cout << "empty vector rows=" << empty_vector.size()
            << " width=" << empty_vector[0].numel() << '\n';
  rejected("matrix rank3", [&] {
    (void)pluss::note::make(std::vector<std::string>{"x"},
                            std::array<std::int64_t, 3>{1, 1, 1}).matrix();
  });
  rejected("matrix undefined", [&] { (void)pluss::Note::undefined().matrix(); });

  const auto cube = pluss::note::make(
      std::vector<std::string>{"000", "001", "010", "011", "100", "101", "110", "111"},
      std::array<std::int64_t, 3>{2, 2, 2});
  const auto t3 = cube.tensor3();
  std::cout << "\n=== tensor3 exact rank ===\n"
            << "shape=";
  print_shape(cube.sizes_view());
  std::cout << " [1][0][1]=" << t3[1].at(1)
            << " [1][1][0]=" << t3[1].at(2) << '\n';
  const auto empty3 = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{2, 0, 3}).tensor3();
  std::cout << "empty slabs=" << empty3.size()
            << " rows=" << empty3[0].size(0) << ',' << empty3[1].size(0) << '\n';
  rejected("tensor3 rank1", [&] { (void)pluss::note::make({"x"}).tensor3(); });
  rejected("tensor3 undefined", [&] { (void)pluss::Note::undefined().tensor3(); });

  std::cout << "\n=== borrowed bytes ===\n";
  const std::array<std::byte, 3> raw{std::byte{'A'}, std::byte{0}, std::byte{'B'}};
  const auto bytes = pluss::note::bytes(raw);
  const auto borrowed = bytes.vector().front();
  std::cout << "size=" << borrowed.size() << " values="
            << static_cast<int>(static_cast<unsigned char>(borrowed[0])) << ','
            << static_cast<int>(static_cast<unsigned char>(borrowed[1])) << ','
            << static_cast<int>(static_cast<unsigned char>(borrowed[2])) << '\n';
  std::string retained;
  {
    const auto temporary = pluss::note::make({"keep", "copy"});
    retained = std::string(temporary.vector()[1]);
  }
  std::cout << "owned copy after source=" << retained << '\n';

  std::cout << "\n=== sizes view ===\nscalar=";
  print_shape(scalar.sizes_view());
  std::cout << " size=" << scalar.sizes_view().size() << "\ngrid=";
  print_shape(grid.sizes_view());
  std::cout << "\nempty=";
  print_shape(empty_matrix.sizes_view());
  std::cout << '\n';
  const auto view = grid.sizes_view();
  const auto copy = grid.sizes();
  std::cout << "view data stable=" << std::boolalpha
            << (view.data() == grid.sizes_view().data())
            << " copy independent=" << (copy.data() != view.data()) << '\n';
  rejected("sizes_view undefined", [&] { (void)pluss::Note::undefined().sizes_view(); });
}
