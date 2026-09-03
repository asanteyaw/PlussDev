#include <pluss/pluss.h>

#include <iostream>

namespace {

void heading(const char* label) {
  std::cout << "=== " << label << " ===\n";
}

const char* tf(bool value) {
  return value ? "true" : "false";
}

} // namespace

int main() {
  heading("to_dtype_widen");
  auto ints = pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64);
  std::cout << ints.to(pluss::ScalarType::Float64) << "\n\n";

  heading("to_dtype_bool");
  auto signed_values = pluss::make_series({0, -2, 5}, pluss::ScalarType::Int64);
  std::cout << pluss::series::print(signed_values.to(pluss::ScalarType::Bool)) << "\n\n";

  heading("to_same_dtype_values");
  auto same_dtype = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << same_dtype.to(pluss::ScalarType::Float64) << "\n\n";

  heading("to_same_platform_values");
  auto same_platform = pluss::make_series({2.0, 4.0, 8.0});
  pluss::series::print(std::cout, same_platform.to(pluss::Platform::CPU)) << "\n\n";

  heading("to_platform_dtype_combined");
  auto decimal = pluss::make_series({1.9, -2.1, 3.5});
  pluss::series::print(std::cout,
                       decimal.to(pluss::Platform::CPU, pluss::ScalarType::Int64))
      << "\n\n";

  heading("to_from_view_becomes_contiguous");
  auto transposed = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  }).transpose(0, 1);
  auto casted_view = transposed.to(pluss::ScalarType::Float32);
  std::cout << "before=" << tf(transposed.is_contiguous()) << "\n";
  std::cout << "after=" << tf(casted_view.is_contiguous()) << "\n";
  std::cout << casted_view << "\n\n";

  heading("to_gradient");
  auto cast_grad = pluss::make_series({1.5, 2.5, 3.5});
  cast_grad.tracks_slope_(true);
  cast_grad.to(pluss::ScalarType::Float32).sum().reverse();
  std::cout << cast_grad.sensitivity() << "\n\n";

  heading("clone_independent_copy");
  auto clone_base = pluss::make_series({1.0, 2.0, 3.0});
  auto clone_copy = clone_base.clone();
  clone_copy.add_(100.0);
  std::cout << "base=" << clone_base << "\n";
  std::cout << "copy=" << clone_copy << "\n\n";

  heading("clone_from_view_materializes");
  auto clone_view = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
      {5.0, 6.0},
  }).transpose(0, 1);
  auto clone_materialized = clone_view.clone();
  std::cout << "before=" << tf(clone_view.is_contiguous()) << "\n";
  std::cout << "after=" << tf(clone_materialized.is_contiguous()) << "\n";
  std::cout << clone_materialized << "\n\n";

  heading("clone_gradient");
  auto clone_grad = pluss::make_series({
      {1.0, 2.0},
      {3.0, 4.0},
  });
  clone_grad.tracks_slope_(true);
  clone_grad.clone().sum().reverse();
  pluss::series::print(std::cout, clone_grad.sensitivity()) << "\n\n";

  heading("contiguous_materializes_view");
  auto contig_view = pluss::make_series({
      {10.0, 20.0},
      {30.0, 40.0},
      {50.0, 60.0},
  }).transpose(0, 1);
  auto contig_copy = contig_view.contiguous();
  contig_copy.add_(7.0);
  std::cout << "view=" << contig_view << "\n";
  std::cout << "copy=" << contig_copy << "\n";
  std::cout << "before=" << tf(contig_view.is_contiguous()) << "\n";
  std::cout << "after=" << tf(contig_copy.is_contiguous()) << "\n\n";

  heading("contiguous_already_contiguous");
  auto already = pluss::make_series({3.0, 6.0, 9.0});
  auto still_contiguous = already.contiguous();
  std::cout << "before=" << tf(already.is_contiguous()) << "\n";
  std::cout << "after=" << tf(still_contiguous.is_contiguous()) << "\n";
  std::cout << still_contiguous << "\n\n";

  heading("contiguous_gradient");
  auto contig_grad = pluss::make_series({
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
  });
  contig_grad.tracks_slope_(true);
  contig_grad.transpose(0, 1).contiguous().sum().reverse();
  std::cout << contig_grad.sensitivity() << "\n\n";

  return 0;
}
