#include <pluss/pluss.h>

#include <cstdint>
#include <iostream>

namespace {

template <class Range>
void print_range(const Range& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}

void describe(std::string_view name, const pluss::Timeline& value) {
  std::cout << name << ": defined=" << value.defined()
            << ", dim=" << value.dim()
            << ", numel=" << value.numel()
            << ", scalar=" << value.is_scalar()
            << ", sizes=";
  print_range(value.sizes());
  std::cout << ", tz=" << value.tz() << '\n';
}

} // namespace

int main() {
  std::cout << std::boolalpha;

  const auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
  const auto vector = pluss::timeline::make({
      "2024-02-29T12:30:00Z",
      "2024-03-01T00:00:00Z",
  });
  const auto one_vector = pluss::timeline::make({"2024-02-29T12:30:00Z"});
  const auto matrix = pluss::timeline::make({
      {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
      {"2024-03-02T06:15:00Z", ""},
  });

  std::cout << "=== shape_and_state ===\n";
  describe("scalar", scalar);
  describe("vector", vector);
  describe("matrix", matrix);

  std::cout << "\n=== missing_is_still_defined ===\n";
  const auto with_nat = pluss::timeline::make({
      "2024-02-29T12:30:00Z",
      "",
  });
  std::cout << "defined=" << with_nat.defined()
            << ", positions=" << with_nat.numel() << '\n';
  pluss::series::print(std::cout, with_nat.isnat());
  std::cout << '\n';

  std::cout << "\n=== empty_shape ===\n";
  const auto empty = pluss::timeline::epoch({2, 0, 4});
  std::cout << "dim=" << empty.dim()
            << ", numel=" << empty.numel()
            << ", empty=" << (empty.numel() == 0) << '\n';

  std::cout << "\n=== epoch_nanos_logical_order ===\n";
  print_range(matrix.ticks());
  std::cout << '\n';
  pluss::series::print(std::cout, matrix.epoch_nanos());
  std::cout << '\n';

  std::cout << "\n=== epoch_round_trip ===\n";
  const auto shaped_epoch = pluss::timeline::epoch({2, 3});
  const auto shaped_ticks = shaped_epoch.ticks();
  const std::vector<std::int64_t> shaped_sizes(
      shaped_epoch.sizes().begin(), shaped_epoch.sizes().end());
  const auto rebuilt = pluss::timeline::from_epoch(shaped_ticks, shaped_sizes);
  std::cout << "source numel=" << shaped_epoch.numel()
            << ", rebuilt dim=" << rebuilt.dim()
            << ", rebuilt numel=" << rebuilt.numel() << '\n';
  std::cout << "series dim=" << shaped_epoch.epoch_nanos().dim()
            << ", series numel=" << shaped_epoch.epoch_nanos().numel()
            << ", vector size=" << shaped_ticks.size() << '\n';

  std::cout << "\n=== numeric_validity_mask ===\n";
  const auto mask_source = pluss::timeline::make({
      "2024-02-29T12:30:00Z", "", "2024-03-01T00:00:00Z"});
  pluss::series::print(
      std::cout, mask_source.epoch_nanos().ne(pluss::Timeline::kNaT));
  std::cout << '\n';

  std::cout << "\n=== host_variants ===\n";
  const auto scalar_host = scalar.host();
  const auto vector_host = vector.host();
  const auto one_vector_host = one_vector.host();
  const auto matrix_host = matrix.host();
  std::cout << "scalar host: scalar=" << scalar_host.is_scalar()
            << ", value=" << scalar_host.scalar()
            << ", tz=" << scalar_host.tz() << '\n';
  std::cout << "vector host: vector=" << vector_host.is_vector()
            << ", values=";
  print_range(vector_host.vector());
  std::cout << ", tz=" << vector_host.tz() << '\n';
  std::cout << "one-element vector: dim=" << one_vector.dim()
            << ", is_scalar=" << one_vector.is_scalar()
            << ", host scalar=" << one_vector_host.is_scalar() << '\n';
  std::cout << "matrix host: array=" << matrix_host.is_array()
            << ", sizes=";
  print_range(matrix_host.array().sizes);
  std::cout << ", values=";
  print_range(matrix_host.array().data);
  std::cout << ", contiguous=" << matrix_host.array().contiguous
            << ", tz=" << matrix_host.array().tz << '\n';

  std::cout << "\n=== timezone_metadata_and_instant ===\n";
  const auto new_york = scalar.tz_convert("America/New_York");
  std::cout << "UTC: tz=" << scalar.tz()
            << ", epoch=" << scalar.ticks().front() << '\n';
  std::cout << "New York: tz=" << new_york.tz()
            << ", epoch=" << new_york.ticks().front() << '\n';
  std::cout << "same instant: "
            << (scalar.ticks() == new_york.ticks()) << '\n';
  std::cout << "UTC ISO: " << scalar.isoformat() << '\n';
  std::cout << "New York ISO: " << new_york.isoformat() << '\n';
}
