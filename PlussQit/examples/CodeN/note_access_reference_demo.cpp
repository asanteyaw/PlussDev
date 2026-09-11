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
void show_rejection(std::string_view label, Fn&& fn) {
  try {
    fn();
    std::cout << label << ": accepted\n";
  } catch (const pluss::Error&) {
    std::cout << label << ": rejected\n";
  }
}

void show_strings(const std::vector<std::string>& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << (i ? ", " : "") << '"' << values[i] << '"';
  }
  std::cout << ']';
}

void show_shape(std::span<const std::int64_t> values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    std::cout << (i ? ", " : "") << values[i];
  }
  std::cout << ']';
}

} // namespace

int main() {
  const auto grid = pluss::note::make(
      std::vector<std::string>{"r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
      std::array<std::int64_t, 2>{2, 3});

  std::cout << "=== at row major ===\n"
            << "at(0)=" << grid.at(0) << '\n'
            << "at(4)=" << grid.at(4) << '\n'
            << "at(5)=" << grid.at(5) << '\n';

  const auto tensor = pluss::note::make(
      std::vector<std::string>{"000", "001", "010", "011", "100", "101", "110", "111"},
      std::array<std::int64_t, 3>{2, 2, 2});
  std::cout << "\n=== at indices ===\n"
            << "matrix[1,2]="
            << grid.at_indices(std::array<std::int64_t, 2>{1, 2}) << '\n'
            << "tensor[1,0,1]="
            << tensor.at_indices(std::array<std::int64_t, 3>{1, 0, 1}) << '\n'
            << "scalar[]="
            << pluss::note::make("solo").at_indices(std::array<std::int64_t, 0>{}) << '\n';

  const std::array<std::byte, 3> raw{
      std::byte{'A'}, std::byte{0}, std::byte{'B'}};
  const auto binary = pluss::note::bytes(raw);
  const auto binary_view = binary.at(0);
  std::cout << "\n=== binary cell ===\n"
            << "bytes=" << binary_view.size()
            << " values=" << static_cast<int>(static_cast<unsigned char>(binary_view[0]))
            << ',' << static_cast<int>(static_cast<unsigned char>(binary_view[1]))
            << ',' << static_cast<int>(static_cast<unsigned char>(binary_view[2])) << '\n';

  pluss::TextRef kept;
  {
    const auto temporary = pluss::note::make({"alpha", "beta"});
    kept = temporary.cell(1);
  }
  std::cout << "\n=== owning cell ===\n"
            << "defined=" << std::boolalpha << kept.defined()
            << " bytes=" << kept.size_bytes()
            << " text=" << kept << '\n'
            << "copy=" << kept.str() << '\n';

  pluss::TextRef coordinate;
  {
    const auto temporary_grid = pluss::note::make({{"north", "east"}, {"south", "west"}});
    coordinate = temporary_grid.cell_at(std::array<std::int64_t, 2>{1, 0});
  }
  std::cout << "coordinate after source=" << coordinate << '\n';

  const auto owned_empty = pluss::note::make("").owned();
  const auto one_matrix = pluss::note::make(
      std::vector<std::string>{"one"}, std::array<std::int64_t, 2>{1, 1});
  std::cout << "\n=== owned one element ===\n"
            << "empty defined=" << owned_empty.defined()
            << " empty=" << owned_empty.empty()
            << " bytes=" << owned_empty.size_bytes() << '\n'
            << "matrix=" << one_matrix.owned() << '\n';

  auto scalar_host = pluss::note::make("alpha").host();
  auto vector_host = pluss::note::make({"a", "b"}).host();
  auto matrix_host = pluss::note::make(
      std::vector<std::string>{"a", "b", "c", "d"},
      std::array<std::int64_t, 2>{2, 2}).host();
  std::cout << "\n=== host ranks ===\n"
            << "scalar kind=" << scalar_host.is_scalar()
            << " value=" << scalar_host.scalar() << '\n'
            << "vector kind=" << vector_host.is_vector() << " values=";
  show_strings(vector_host.vector());
  std::cout << "\narray kind=" << matrix_host.is_array() << " shape=";
  show_shape(matrix_host.array().shape());
  std::cout << " values=";
  show_strings(matrix_host.array().data);
  std::cout << " contiguous=" << matrix_host.array().contiguous << '\n';

  pluss::Note::HostAny retained_host;
  {
    const auto temporary = pluss::note::make({"kept", "copy"});
    retained_host = temporary.host();
  }
  std::cout << "retained=";
  show_strings(retained_host.vector());
  std::cout << '\n';

  auto empty_host = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 2>{3, 0}).host();
  std::cout << "\n=== host empty geometry ===\nshape=";
  show_shape(empty_host.array().shape());
  std::cout << " values=" << empty_host.array().numel() << '\n';

  std::cout << "\n=== access failures ===\n";
  show_rejection("at(-1)", [&] { (void)grid.at(-1); });
  show_rejection("at(6)", [&] { (void)grid.at(6); });
  show_rejection("rank mismatch", [&] {
    (void)grid.at_indices(std::array<std::int64_t, 1>{0});
  });
  show_rejection("index out of bounds", [&] {
    (void)grid.cell_at(std::array<std::int64_t, 2>{2, 0});
  });
  show_rejection("owned multi-cell", [&] { (void)grid.owned(); });
  show_rejection("host undefined", [&] { (void)pluss::Note::undefined().host(); });
  show_rejection("wrong host accessor", [&] { (void)vector_host.scalar(); });

  std::cout << "\n=== at scalar ===\n"
            << pluss::note::make("solo").at(0) << '\n';

  std::string retained_borrow;
  {
    const auto temporary = pluss::note::make({"alpha", "beta"});
    retained_borrow = std::string(temporary.at(1));
  }
  std::cout << "\n=== at copied borrow ===\n" << retained_borrow << '\n';

  std::cout << "\n=== at short failures ===\n";
  const auto short_values = pluss::note::make({"a", "b"});
  for (const std::int64_t index : {-1, 2}) {
    try {
      (void)short_values.at(index);
    } catch (const pluss::Error&) {
      std::cout << "at(" << index << "): rejected\n";
    }
  }

  const auto small_grid = pluss::note::make({{"a", "b"}, {"c", "d"}});
  const std::array<std::int64_t, 2> fixed{1, 0};
  const std::vector<std::int64_t> dynamic{1, 1};
  const std::span<const std::int64_t> borrowed(dynamic);
  std::cout << "\n=== coordinate containers ===\n"
            << small_grid.at_indices(fixed) << ' '
            << small_grid.at_indices(dynamic) << ' '
            << small_grid.at_indices(borrowed) << '\n';

  const auto delta = pluss::note::make("delta").cell(0);
  const std::string_view delta_view = delta;
  const std::string delta_copy = delta.str();
  std::cout << "\n=== TextRef projections ===\n"
            << "view=" << delta_view
            << " copy=" << delta_copy
            << " first=" << delta[0] << '\n';

  std::cout << "\n=== TextRef iteration ===\n";
  for (const unsigned char byte : pluss::note::make("ABC").cell(0)) {
    std::cout << static_cast<int>(byte) << ' ';
  }
  std::cout << '\n';

  const auto empty_ref = pluss::note::make("").cell(0);
  const pluss::TextRef absent_ref;
  std::cout << "\n=== TextRef empty states ===\n"
            << "empty: defined=" << empty_ref.defined()
            << " empty=" << empty_ref.empty() << '\n'
            << "default: defined=" << absent_ref.defined()
            << " empty=" << absent_ref.empty() << '\n';

  const auto binary_ref = binary.cell(0);
  std::cout << "\n=== TextRef binary ===\n"
            << "bytes=" << binary_ref.size_bytes()
            << " middle=" << static_cast<int>(
                   static_cast<unsigned char>(binary_ref[1])) << '\n';

  std::cout << "\n=== cell short failures ===\n";
  for (const std::int64_t index : {-1, 2}) {
    try {
      (void)short_values.cell(index);
    } catch (const pluss::Error&) {
      std::cout << "cell(" << index << "): rejected\n";
    }
  }

  const auto compass = pluss::note::make({{"north", "east"}, {"south", "west"}});
  std::cout << "\n=== cell_at matrix ===\n"
            << compass.cell_at(std::array<std::int64_t, 2>{1, 0}) << '\n';

  const auto origin = pluss::note::make("origin");
  std::cout << "\n=== cell_at ranks ===\n"
            << origin.cell_at(std::array<std::int64_t, 0>{}) << ' '
            << tensor.cell_at(std::array<std::int64_t, 3>{1, 1, 0}) << '\n';

  const std::vector<std::int64_t> coordinate_vector{0, 1};
  std::cout << "\n=== cell_at dynamic ===\n"
            << small_grid.cell_at(coordinate_vector) << '\n';

  std::cout << "\n=== cell_at failures ===\n";
  auto reject_coordinate = [&](auto coordinate_value) {
    try {
      (void)small_grid.cell_at(coordinate_value);
    } catch (const pluss::Error&) {
      std::cout << "rejected\n";
    }
  };
  reject_coordinate(std::array<std::int64_t, 1>{0});
  reject_coordinate(std::array<std::int64_t, 2>{0, -1});
  reject_coordinate(std::array<std::int64_t, 2>{2, 0});

  std::cout << "\n=== host vector ===\n"
            << "vector kind=" << vector_host.is_vector()
            << " size=" << vector_host.vector().size()
            << " last=" << vector_host.vector().back() << '\n';

  const auto tensor_host = pluss::note::make(
      std::vector<std::string>{"000", "001", "010", "011"},
      std::array<std::int64_t, 3>{1, 2, 2}).host();
  std::cout << "\n=== host tensor ===\n"
            << "dim=" << tensor_host.array().dim()
            << " numel=" << tensor_host.array().numel()
            << " shape=[" << tensor_host.array().sizes[0] << ", "
            << tensor_host.array().sizes[1] << ", "
            << tensor_host.array().sizes[2] << "]\n";

  auto editable = retained_host.vector();
  editable[0] = "changed";
  std::cout << "\n=== host independent copy ===\n"
            << retained_host.vector()[0] << ' ' << editable[0] << '\n';

  std::cout << "\n=== owned scalar ===\n"
            << pluss::note::make("solo").owned() << '\n';

  const auto zero = pluss::note::make(
      std::vector<std::string>{}, std::array<std::int64_t, 1>{0});
  const auto many = pluss::note::make({"a", "b"});
  std::cout << "\n=== owned failures ===\n";
  auto reject_owned = [](std::string_view label, const pluss::Note& note) {
    try {
      (void)note.owned();
    } catch (const pluss::Error&) {
      std::cout << label << ": rejected\n";
    }
  };
  reject_owned("zero", zero);
  reject_owned("many", many);
  reject_owned("undefined", pluss::Note::undefined());
}
