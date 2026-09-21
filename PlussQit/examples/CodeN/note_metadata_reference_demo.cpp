#include <pluss/pluss.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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

template <class T>
void show_lanes(std::span<const T> lanes) {
  std::cout << '[';
  for (std::size_t i = 0; i < lanes.size(); ++i) {
    if (i) std::cout << ", ";
    if constexpr (std::is_same_v<T, std::uint8_t>)
      std::cout << (lanes[i] ? "true" : "false");
    else
      std::cout << lanes[i];
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
            << " rank3=" << value.is_rank3() << '\n';
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
  const auto bytes = pluss::note::bytes(std::string_view{"A\0B", 3});
  const auto flags = pluss::note::make(std::vector<bool>{true, false});
  const auto counts = pluss::note::make(std::vector<std::int64_t>{2, 5});
  const auto scores = pluss::note::make(std::vector<double>{0.25, 0.75});

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

  std::cout << "\n=== dtype inventory ===\n";
  for (const auto& [label, value] :
       std::array<std::pair<std::string_view, const pluss::Note*>, 5>{{
           {"text", &scalar}, {"bytes", &bytes}, {"bool", &flags},
           {"int", &counts}, {"float", &scores}}}) {
    std::cout << label << ": dtype=" << pluss::coden::dtype_name(value->dtype())
              << " text=" << value->is_text()
              << " bytes=" << value->is_bytes()
              << " bool=" << value->is_bool()
              << " int64=" << value->is_int64()
              << " float64=" << value->is_float64() << '\n';
  }

  std::cout << "\n=== dtype shape independence ===\n";
  const auto bool_grid = pluss::note::make(
      std::vector<bool>{true, false, false, true},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << "dtype=" << pluss::coden::dtype_name(bool_grid.dtype())
            << " dim=" << bool_grid.dim()
            << " shape=[" << bool_grid.size(0) << ", " << bool_grid.size(1)
            << "] bool=" << bool_grid.is_bool() << '\n';

  std::cout << "\n=== undefined dtype queries ===\n"
            << "text=" << undefined.is_text()
            << " bytes=" << undefined.is_bytes()
            << " bool=" << undefined.is_bool()
            << " int64=" << undefined.is_int64()
            << " float64=" << undefined.is_float64() << '\n';
  try {
    (void)undefined.dtype();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }

  std::cout << "\n=== is_bool examples ===\n"
            << "flags=" << flags.is_bool()
            << " text=" << scalar.is_bool()
            << " absent=" << undefined.is_bool() << '\n'
            << "predicate=" << scalar.contains("a").is_bool()
            << " shape=[" << bool_grid.size(0) << ", " << bool_grid.size(1)
            << "]\n";

  std::cout << "\n=== is_float64 examples ===\n"
            << "scores=" << scores.is_float64()
            << " integers=" << counts.is_float64()
            << " absent=" << undefined.is_float64() << '\n';

  std::cout << "\n=== is_int64 examples ===\n"
            << "counts=" << counts.is_int64()
            << " floats=" << scores.is_int64()
            << " absent=" << undefined.is_int64() << '\n'
            << "measurement=" << scalar.lengths_chars().is_int64() << '\n';

  std::cout << "\n=== is_text examples ===\n"
            << "text=" << scalar.is_text()
            << " bytes=" << bytes.is_text()
            << " absent=" << undefined.is_text() << '\n'
            << "lowered=" << scalar.lower().is_text() << '\n';

  std::cout << "\n=== dtype page examples ===\n"
            << pluss::coden::dtype_name(scalar.dtype()) << ' '
            << pluss::coden::dtype_name(bytes.dtype()) << ' '
            << pluss::coden::dtype_name(flags.dtype()) << ' '
            << pluss::coden::dtype_name(counts.dtype()) << ' '
            << pluss::coden::dtype_name(scores.dtype()) << '\n'
            << "dtype=" << pluss::coden::dtype_name(bool_grid.dtype())
            << " dim=" << bool_grid.dim()
            << " shape=[" << bool_grid.size(0) << ", " << bool_grid.size(1)
            << "]\n";
  try {
    (void)undefined.dtype();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }

  std::cout << "\n=== predicate page examples ===\n"
            << "flags=" << flags.is_bool()
            << " text=" << scalar.is_bool()
            << " absent=" << undefined.is_bool() << '\n'
            << "predicate=" << scalar.contains("a").is_bool() << '\n'
            << "bool=" << bool_grid.is_bool()
            << " shape=[" << bool_grid.size(0) << ", " << bool_grid.size(1)
            << "]\n"
            << "scores=" << scores.is_float64()
            << " integers=" << counts.is_float64() << '\n'
            << "absent=" << undefined.is_float64() << '\n';
  const auto special_reals = pluss::note::make(
      std::vector<double>{0.0, std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::quiet_NaN()});
  std::cout << special_reals.is_float64()
            << " lanes=" << special_reals.numel() << '\n'
            << "counts=" << counts.is_int64()
            << " floats=" << scores.is_int64()
            << " absent=" << undefined.is_int64() << '\n';
  const auto measured = scalar.lengths_chars();
  std::cout << "measurement=" << measured.is_int64()
            << " value=" << measured.int_item() << '\n';
  const auto int_grid = pluss::note::make(
      std::vector<std::int64_t>{1, 2, 3, 4},
      std::array<std::int64_t, 2>{2, 2});
  std::cout << int_grid.is_int64() << " dim=" << int_grid.dim() << '\n'
            << "text=" << scalar.is_text()
            << " bytes=" << bytes.is_text()
            << " absent=" << undefined.is_text() << '\n';
  const auto lowered = pluss::note::make("Alpha").lower();
  std::cout << "lowered=" << lowered.is_text()
            << " value=" << lowered.scalar() << '\n';
  const auto mask = pluss::note::make({"alpha", "beta"}).contains("a");
  std::cout << "text=" << mask.is_text()
            << " bool=" << mask.is_bool() << '\n';

  const auto one_bool_grid = pluss::note::make(
      std::vector<bool>{true}, std::array<std::int64_t, 3>{1, 1, 1});
  std::cout << "\n=== bool_item page examples ===\n"
            << "scalar=" << pluss::note::make(false).bool_item()
            << " rank3=" << one_bool_grid.bool_item() << '\n'
            << "pipeline="
            << pluss::note::make("chapter").contains("apt").bool_item() << '\n';
  show_rejection("bool_item empty", [&] {
    (void)pluss::note::make(std::vector<bool>{}).bool_item();
  });
  show_rejection("bool_item multiple", [&] { (void)flags.bool_item(); });
  show_rejection("bool_item wrong dtype", [&] {
    (void)pluss::note::make(std::int64_t{1}).bool_item();
  });
  show_rejection("bool_item undefined", [&] { (void)undefined.bool_item(); });

  const auto one_int_grid = pluss::note::make(
      std::vector<std::int64_t>{-42}, std::array<std::int64_t, 2>{1, 1});
  std::cout << "\n=== int_item page examples ===\n"
            << "scalar=" << pluss::note::make(std::int64_t{7}).int_item()
            << " rank2=" << one_int_grid.int_item() << '\n'
            << "measurement="
            << pluss::note::make("café").lengths_chars().int_item() << '\n'
            << "limits=" << pluss::note::make(std::numeric_limits<std::int64_t>::min()).int_item()
            << ',' << pluss::note::make(std::numeric_limits<std::int64_t>::max()).int_item()
            << '\n';
  show_rejection("int_item empty", [&] {
    (void)pluss::note::make(std::vector<std::int64_t>{}).int_item();
  });
  show_rejection("int_item multiple", [&] { (void)counts.int_item(); });
  show_rejection("int_item wrong dtype", [&] {
    (void)pluss::note::make(1.0).int_item();
  });
  show_rejection("int_item undefined", [&] { (void)undefined.int_item(); });

  const auto one_real_grid = pluss::note::make(
      std::vector<double>{1.25}, std::array<std::int64_t, 3>{1, 1, 1});
  std::cout << "\n=== float_item page examples ===\n"
            << "scalar=" << pluss::note::make(0.125).float_item()
            << " rank3=" << one_real_grid.float_item() << '\n'
            << std::setprecision(std::numeric_limits<double>::max_digits10)
            << "precision=" << pluss::note::make(1.0 / 10.0).float_item() << '\n'
            << "special=" << std::signbit(pluss::note::make(-0.0).float_item())
            << ',' << std::isinf(pluss::note::make(
                   std::numeric_limits<double>::infinity()).float_item())
            << ',' << std::isnan(pluss::note::make(
                   std::numeric_limits<double>::quiet_NaN()).float_item()) << '\n';
  show_rejection("float_item empty", [&] {
    (void)pluss::note::make(std::vector<double>{}).float_item();
  });
  show_rejection("float_item multiple", [&] { (void)scores.float_item(); });
  show_rejection("float_item wrong dtype", [&] {
    (void)pluss::note::make(std::int64_t{1}).float_item();
  });
  show_rejection("float_item undefined", [&] { (void)undefined.float_item(); });

  std::cout << "\n=== bools page examples ===\n";
  show_lanes(flags.bools());
  std::cout << " size=" << flags.bools().size() << '\n';
  show_lanes(bool_grid.bools());
  std::cout << " shape=[" << bool_grid.size(0) << ", " << bool_grid.size(1)
            << "]\n";
  const auto flags_copy = flags;
  std::cout << "shared="
            << (flags.bools().data() == flags_copy.bools().data()) << '\n';
  const auto empty_bools = pluss::note::make(
      std::vector<bool>{}, std::array<std::int64_t, 2>{0, 3});
  std::cout << "empty lanes=" << empty_bools.bools().size()
            << " shape=[" << empty_bools.size(0) << ", "
            << empty_bools.size(1) << "]\n";
  try {
    (void)scalar.bools();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }
  try {
    (void)undefined.bools();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }

  std::cout << "\n=== integers page examples ===\n";
  show_lanes(counts.integers());
  std::cout << " size=" << counts.integers().size() << '\n';
  show_lanes(int_grid.integers());
  std::cout << " shape=[" << int_grid.size(0) << ", " << int_grid.size(1)
            << "]\n";
  pluss::Note retained_row = pluss::Note::undefined();
  {
    const auto source = pluss::note::make(
        std::vector<std::int64_t>{10, 11, 12, 20, 21, 22},
        std::array<std::int64_t, 2>{2, 3});
    retained_row = source.matrix()[1];
  }
  show_lanes(retained_row.integers());
  std::cout << " retained-after-source\n";
  try {
    (void)flags.integers();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }
  try {
    (void)undefined.integers();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }

  std::cout << "\n=== reals page examples ===\n";
  show_lanes(scores.reals());
  std::cout << " size=" << scores.reals().size() << '\n';
  const auto real_grid = pluss::note::make(
      std::vector<double>{0.5, 1.5, 2.5, 3.5},
      std::array<std::int64_t, 2>{2, 2});
  show_lanes(real_grid.reals());
  std::cout << " shape=[" << real_grid.size(0) << ", "
            << real_grid.size(1) << "]\n";
  const auto empty_reals = pluss::note::make(
      std::vector<double>{}, std::array<std::int64_t, 1>{0});
  std::cout << "empty lanes=" << empty_reals.reals().size() << '\n';
  try {
    (void)counts.reals();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }
  try {
    (void)undefined.reals();
  } catch (const pluss::Error& error) {
    const std::string message(error.what());
    std::cout << message.substr(0, message.find('\n')) << '\n';
  }

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
  std::cout << "\n=== rank3 examples ===\n"
            << "rank3=" << tensor.is_rank3() << " dim=" << tensor.dim()
            << " numel=" << tensor.numel() << '\n'
            << "rank3=" << zero_tensor.is_rank3()
            << " empty=" << zero_tensor.empty()
            << " last=" << zero_tensor.size(-1) << '\n'
            << "matrix=" << singleton_matrix.is_matrix()
            << " rank3=" << singleton_matrix.is_rank3() << '\n'
            << "defined=" << undefined.defined()
            << " rank3=" << undefined.is_rank3() << '\n';
}
