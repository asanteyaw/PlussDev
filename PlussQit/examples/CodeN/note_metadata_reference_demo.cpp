#include <pluss/pluss.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void show_shape(const pluss::Note& value) {
  const auto shape = value.sizes();
  std::cout << '[';
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << shape[i];
  }
  std::cout << ']';
}

void show_metadata(std::string_view label, const pluss::Note& value) {
  std::cout << label << ": defined=" << value.defined()
            << " dim=" << value.dim() << " sizes=";
  if (value.defined()) {
    show_shape(value);
  } else {
    std::cout << "unavailable";
  }
  std::cout << " numel=" << value.numel()
            << " empty=" << value.empty()
            << " scalar=" << value.is_scalar()
            << " vector=" << value.is_vector()
            << " matrix=" << value.is_matrix()
            << " tensor3=" << value.is_tensor3() << '\n';
}

template <typename Fn>
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

} // namespace

int main() {
  std::cout << std::boolalpha;

  const auto scalar = pluss::note::make("alpha");
  const auto vector = pluss::note::make({"alpha"});
  const auto matrix = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f"},
      std::array<std::int64_t, 2>{2, 3});
  const auto tensor = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"},
      std::array<std::int64_t, 3>{2, 2, 2});
  const auto empty = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{2, 0});
  const auto undefined = pluss::Note::undefined();

  std::cout << "=== rank inventory ===\n";
  show_metadata("scalar", scalar);
  show_metadata("singleton vector", vector);
  show_metadata("matrix", matrix);
  show_metadata("tensor", tensor);
  show_metadata("empty matrix", empty);
  show_metadata("undefined", undefined);

  std::cout << "\n=== dimensions ===\n"
            << "matrix.size(0)=" << matrix.size(0) << '\n'
            << "matrix.size(1)=" << matrix.size(1) << '\n'
            << "matrix.size(-1)=" << matrix.size(-1) << '\n'
            << "matrix.size(-2)=" << matrix.size(-2) << '\n';

  std::cout << "\n=== failures ===\n";
  show_rejection("matrix.size(2)", [&] { (void)matrix.size(2); });
  show_rejection("matrix.size(-3)", [&] { (void)matrix.size(-3); });
  show_rejection("undefined.size(0)", [&] { (void)undefined.size(0); });
  show_rejection("undefined.sizes()", [&] { (void)undefined.sizes(); });

  std::cout << "\n=== defined examples ===\n";
  const auto empty_text = pluss::note::make("");
  std::cout << "absent=" << undefined.defined() << '\n'
            << "empty text=" << empty_text.defined() << '\n'
            << "empty matrix=" << empty.defined() << '\n';
  if (!undefined.defined()) {
    std::cout << "no Note supplied\n";
  }
  std::cout << "defined=" << undefined.defined()
            << " dim=" << undefined.dim()
            << " scalar=" << undefined.is_scalar() << '\n';

  std::cout << "\n=== dim examples ===\n"
            << scalar.dim() << ' ' << vector.dim() << ' '
            << matrix.dim() << ' ' << tensor.dim() << '\n'
            << "dim=" << empty.dim() << " numel=" << empty.numel()
            << " matrix=" << empty.is_matrix() << '\n'
            << "defined=" << undefined.defined() << " dim=" << undefined.dim()
            << " scalar=" << undefined.is_scalar() << '\n';

  std::cout << "\n=== empty examples ===\n"
            << "empty=" << empty_text.empty() << " numel=" << empty_text.numel()
            << " scalar=" << empty_text.is_scalar() << '\n'
            << "empty=" << empty.empty() << " dim=" << empty.dim()
            << " sizes=[" << empty.size(0) << ", " << empty.size(1) << "]\n";
  const auto empty_vector = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
  std::cout << "empty value: defined=" << empty_vector.defined()
            << " empty=" << empty_vector.empty() << '\n'
            << "absent: defined=" << undefined.defined()
            << " empty=" << undefined.empty() << '\n';

  std::cout << "\n=== size examples ===\n"
            << "rows=" << empty.size(0) << " columns=" << empty.size(1)
            << " cells=" << empty.numel() << '\n';

  std::cout << "\n=== sizes examples ===\n";
  show_shape(scalar); std::cout << '\n';
  show_shape(vector); std::cout << '\n';
  show_shape(pluss::note::make({{"a", "b"}})); std::cout << '\n';
  auto shape_copy = matrix.sizes();
  shape_copy[0] = 99;
  std::cout << "copy=[" << shape_copy[0] << ", " << shape_copy[1] << "] "
            << "note=[" << matrix.size(0) << ", " << matrix.size(1) << "]\n"
            << "empty sizes=[" << empty.size(0) << ", " << empty.size(1) << "]\n";
  show_rejection("undefined.sizes()", [&] { (void)undefined.sizes(); });

  std::cout << "\n=== numel examples ===\n"
            << "scalar=" << scalar.numel() << " singleton=" << vector.numel()
            << " matrix=" << matrix.numel() << '\n';
  const auto empty_tensor = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{4, 0, 7});
  std::cout << "dim=" << empty_tensor.dim() << " numel=" << empty_tensor.numel()
            << " empty=" << empty_tensor.empty() << '\n'
            << "empty: defined=" << empty_vector.defined()
            << " numel=" << empty_vector.numel() << '\n'
            << "absent: defined=" << undefined.defined()
            << " numel=" << undefined.numel() << '\n';

  std::cout << "\n=== scalar examples ===\n"
            << "scalar: " << scalar.is_scalar()
            << " sizes=" << scalar.sizes().size() << '\n'
            << "singleton: " << vector.is_scalar()
            << " dim=" << vector.dim() << '\n'
            << "scalar=" << empty_text.is_scalar()
            << " empty=" << empty_text.empty()
            << " bytes=" << empty_text.lengths_bytes().int_item() << '\n'
            << "defined=" << undefined.defined() << " dim=" << undefined.dim()
            << " scalar=" << undefined.is_scalar() << '\n';

  std::cout << "\n=== vector examples ===\n"
            << "scalar vector=" << scalar.is_vector() << '\n'
            << "[1] vector=" << vector.is_vector() << '\n'
            << "vector=" << empty_vector.is_vector()
            << " empty=" << empty_vector.empty()
            << " size=" << empty_vector.size(0) << '\n'
            << "matrix=" << matrix.is_vector()
            << " absent=" << undefined.is_vector() << '\n';

  const auto singleton_matrix = pluss::note::make(
      std::vector<std::string>{"only"}, std::array<std::int64_t, 2>{1, 1});
  std::cout << "\n=== matrix examples ===\n"
            << "matrix=" << matrix.is_matrix() << " rows=" << matrix.size(0)
            << " columns=" << matrix.size(1) << '\n'
            << "matrix=" << singleton_matrix.is_matrix()
            << " scalar=" << singleton_matrix.is_scalar()
            << " numel=" << singleton_matrix.numel() << '\n'
            << "matrix=" << empty.is_matrix() << " empty=" << empty.empty()
            << " numel=" << empty.numel() << '\n';

  const auto zero_tensor = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 3>{1, 2, 0});
  std::cout << "\n=== tensor3 examples ===\n"
            << "tensor3=" << tensor.is_tensor3() << " dim=" << tensor.dim()
            << " numel=" << tensor.numel() << '\n'
            << "tensor3=" << zero_tensor.is_tensor3()
            << " empty=" << zero_tensor.empty()
            << " last=" << zero_tensor.size(-1) << '\n'
            << "matrix=" << singleton_matrix.is_matrix()
            << " tensor3=" << singleton_matrix.is_tensor3() << '\n';
}
