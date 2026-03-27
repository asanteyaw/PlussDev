#include <pluss/pluss.h>
#include <pluss/tils.h>

#include <complex>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

int main() {
  auto mps_ready = pluss::mps::is_ready();
  pluss::Platform platform(mps_ready ? pluss::pMPS : pluss::pCPU);
  std::cout << (mps_ready ? "MPS ready. Training on GPU." : "Training on CPU.") << '\n';

  pluss::Series a = pluss::make_series({1.0, 2.0, 3.0}).to(platform);
  std::cout << "Series a:\n" << a << "\n";
  std::cout << "a dtype: " << a.dtype() << "\n";
  std::cout << "a shape: " << a.sizes() << "\n";
  std::cout << "a platform: " << a.platform() << "\n";
  
#ifdef PLUSS_ENABLE_TORCH
  torch::Tensor A = torch::tensor({
        {1.0, 2.0, 3.0},
        {4.0, 5.0, 6.0}
    });
  let a2 = pluss::from_torch(A);
  torch::Tensor B = torch::tensor({
        {
            {1.0, 2.0},
            {3.0, 4.0}
        },
        {
            {5.0, 6.0},
            {7.0, 8.0}
        }
    });
  let b2 = pluss::from_torch(B);
#endif
  std::vector<std::int64_t> raw{10, 20, 30, 40};
  pluss::Series b = pluss::make_series(raw, {2, 2}).to(platform);

  const pluss::Platform p = a.platform();
  std::cout << "a platform enum=" << static_cast<int>(p) << "\n";
  auto a_cpu_f32 = a.to(pluss::Platform::CPU, pluss::ScalarType::Float32);
  std::cout << "a.to(CPU, Float32) numel=" << a_cpu_f32.numel() << "\n";
  auto default_cpu = pluss::zeros({2, 2});
  std::cout << "zeros default platform enum=" << static_cast<int>(default_cpu.platform()) << "\n";
  if (pluss::mps::is_ready()) {
    auto created_mps = pluss::zeros({2, 2}, pluss::cfg::platform(pluss::Platform::MPS));
    std::cout << "zeros(..., cfg::platform(MPS)) platform enum="
              << static_cast<int>(created_mps.platform()) << "\n";
  }
  if (pluss::mps::is_ready()) {
    auto a_mps = a.to(pluss::Platform::MPS);
    std::cout << "a.to(MPS) platform enum=" << static_cast<int>(a_mps.platform()) << "\n";
    auto a_mps_back = a_mps.to(pluss::Platform::CPU);
    std::cout << "a.to(MPS).to(CPU) first=" << a_mps_back.to_vec<double>(0) << "\n";
  } else {
    std::cout << "MPS backend not available in this build/runtime\n";
  }

  auto c = a.add(2.0).mul(3.0);
  auto d = b.sum();

  std::cout << "a numel=" << a.numel() << " c numel=" << c.numel() << "\n";
  std::cout << "b shape=(2,2) d=" << d.to_vec<std::int64_t>()[0] << "\n";

  std::cout << "looking at 1D rendering\n" << pluss::print_series(a) << "\n";
  std::cout << "looking at 0D rendering\n" << pluss::print_series(d) << "\n";

  let rv = pluss::normal(0.0, 1.0, {5});
  std::cout << "random values: " << rv << "\n";
  std::cout << "sum: " << rv.sum() << "\n";
  std::cout << "mean: " << rv.mean() << "\n";
  std::cout << "std: " << rv.std() << "\n";

  let e = c + d;
  std::cout << "e: " << e-a << "\n";

  let rand_vals = pluss::randn({3, 4});
  std::cout << "randn(3,4) default:\n" << rand_vals << "\n";
  std::cout << "to vec:\n";
  const auto all_vals = rand_vals.to_vec<double>();
  std::cout << "all: [";
  for (std::size_t i = 0; i < all_vals.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << all_vals[i];
  }
  std::cout << "]\n";
  std::cout << "first via to_vec(index)=" << rand_vals.to_vec<double>(0) << "\n";
  auto h = rand_vals.host<double>().array();
  std::cout << "host shape=(" << h.shape()[0] << "," << h.shape()[1] << ") numel=" << h.numel() << "\n";
  std::cout << "first=" << h.data.at(0) << "\n";
  auto hb = rand_vals.host<bool>().array();
  std::size_t true_count = 0;
  for (bool v : hb.data) {
    if (v) ++true_count;
  }
  std::cout << "bool host true_count=" << true_count << "\n";

#ifdef PLUSS_ENABLE_TORCH
  std::cout << "looking at the from_torch conversion\n" << a2 << "\n";
  std::cout << "looking at the from_torch conversion\n" << b2 << "\n";
  std::cout << "looking at the from_torch conversion\n" << pluss::print_series(a2) << "\n";
  std::cout << "looking at the from_torch conversion\n" << pluss::print_series(b2) << "\n";
#endif

  std::cout << "randn(3,4) pretty default:\n" << pluss::print_series(rand_vals) << "\n";

  let rand_3d = pluss::randn({3, 4, 5});
  std::cout << "randn(3,4,5) pretty:\n" << pluss::print_series(rand_3d) << "\n";

  let big_mat = pluss::randn({8, 6});
  std::cout << "randn(8,6) default with row/col summary:\n" << big_mat << "\n";
  std::cout << "randn(8,6) pretty summarized:\n" << pluss::print_series(big_mat) << "\n";

#ifdef PLUSS_ENABLE_TORCH
  let torch_b = pluss::to_torch(rand_3d);
  std::cout << "looking at the to_torch conversion\n" << torch_b << "\n";
#endif

  let type_prom_2d = pluss::make_series ({
    {1, 2.5, 3},
    {4.75, 5, 6.125}
      });
  let type_prom_3d = pluss::make_series({
    {
      {1, 2.5, 3},
      {4.75, 5, 6.125}
    },
    {
      {7, 8.25, 9},
      {10.5, 11, 12.625}
    }
  });

  std::cout << "Mixed types 2D\n" << pluss::print_series(type_prom_2d) << "\n";
  std::cout << "Mixed types 3D\n" << pluss::print_series(type_prom_3d) << "\n";

  // 7D demos (rank-7): one rvalue literal path, one lvalue host-buffer path.
  let seven_d_rvalue = pluss::make_series({
    {
      {
        {
          {
            {
              {1.0, 2.0, 3.0},
              {4.0, 5.0, 6.0}
            }
          }
        }
      }
    }
  }); // shape=[1,1,1,1,1,2,3]
  std::cout << "7D rvalue via value(...):\n" << pluss::print_series(seven_d_rvalue) << "\n";

  std::vector<double> seven_d_host{10.0, 11.0, 12.0, 13.0, 14.0, 15.0};
  let seven_d_lvalue = pluss::make_series(seven_d_host, {1, 1, 1, 1, 1, 2, 3});
  std::cout << "7D lvalue via make_series(host, sizes):\n" << pluss::print_series(seven_d_lvalue) << "\n";

  let large_2d = pluss::randn({11, 7});
  std::cout << "large 2D with default summarization:\n" << pluss::print_series(large_2d) << "\n";

  // 1) Precision / notation / trailing-zero trim
  pluss::PrintOptions fixed_fmt;
  fixed_fmt.number_mode = pluss::codem::PrintNumberMode::Fixed;
  fixed_fmt.precision = 4;
  fixed_fmt.trim_trailing_zeros = true;
  std::cout << "fixed + trim:\n" << pluss::print_series(rand_vals, fixed_fmt) << "\n";

  pluss::PrintOptions sci_fmt;
  sci_fmt.number_mode = pluss::codem::PrintNumberMode::Scientific;
  sci_fmt.precision = 3;
  std::cout << "scientific:\n" << pluss::print_series(rand_vals, sci_fmt) << "\n";

  // 2) Matrix layout: alignment / bracket style / separators
  pluss::PrintOptions decimal_align;
  decimal_align.pretty_align = pluss::codem::PrintAlignMode::Decimal;
  decimal_align.pretty_values_with_commas = true;
  std::cout << "decimal align + comma separator:\n" << pluss::print_series(rand_vals, decimal_align) << "\n";

  pluss::PrintOptions ascii_brackets;
  ascii_brackets.pretty_bracket_style = pluss::codem::PrintBracketStyle::Ascii;
  std::cout << "ascii brackets:\n" << pluss::print_series(rand_vals, ascii_brackets) << "\n";

  pluss::PrintOptions no_brackets;
  no_brackets.pretty_bracket_style = pluss::codem::PrintBracketStyle::None;
  no_brackets.pretty_align = pluss::codem::PrintAlignMode::Left;
  std::cout << "no brackets + left align:\n" << pluss::print_series(rand_vals, no_brackets) << "\n";

  // 3) Summarization policy + line-width based reduction
  pluss::PrintOptions line_limited;
  line_limited.pretty_max_line_width = 40;
  line_limited.pretty_matrix_max_cols = 0;
  std::cout << "line-width summarized:\n" << pluss::print_series(big_mat, line_limited) << "\n";

  // 4) Slice rendering controls
  pluss::PrintOptions slice_style;
  slice_style.pretty_show_slice_indices = true;
  slice_style.pretty_slice_label_prefix = "slice";
  slice_style.pretty_slice_label_suffix = ":";
  slice_style.pretty_max_slices = 4;
  slice_style.pretty_slice_sampling = pluss::codem::PrintSliceSampling::Uniform;
  std::cout << "3D with custom slice labels + uniform sampling:\n"
            << pluss::print_series(rand_3d, slice_style) << "\n";

  pluss::PrintOptions hide_slice_labels;
  hide_slice_labels.pretty_show_slice_indices = false;
  std::cout << "3D no slice labels:\n" << pluss::print_series(rand_3d, hide_slice_labels) << "\n";

  // 5) Metadata/content verbosity controls
  pluss::PrintOptions compact_meta;
  compact_meta.metadata_mode = pluss::codem::PrintMetadataMode::Compact;
  std::cout << "compact metadata:\n" << pluss::print_series(rand_vals, compact_meta) << "\n";

  pluss::PrintOptions values_only;
  values_only.metadata_mode = pluss::codem::PrintMetadataMode::None;
  values_only.content_mode = pluss::codem::PrintContentMode::ValuesOnly;
  std::cout << "values only:\n" << pluss::print_series(rand_vals, values_only) << "\n";

  pluss::PrintOptions header_only;
  header_only.content_mode = pluss::codem::PrintContentMode::HeaderOnly;
  std::cout << "header only:\n" << pluss::print_series(rand_vals, header_only) << "\n";

  // 6) Width controls: clip cell width + auto-wrap
  let wide_mat = pluss::randn({4, 12});
  pluss::PrintOptions wrapped;
  wrapped.pretty_matrix_max_cols = 0;
  wrapped.pretty_max_line_width = 44;
  wrapped.pretty_auto_wrap = true;
  wrapped.pretty_cell_max_width = 9;
  std::cout << "wrapped + clipped wide matrix:\n" << pluss::print_series(wide_mat, wrapped) << "\n";

  // 7) Type-specific bool/complex rendering
  let bool_series = rand_vals.gt(0.0);
  pluss::PrintOptions bool01;
  bool01.bool_style = pluss::codem::PrintBoolStyle::OneZero;
  std::cout << "bool as 0/1:\n" << pluss::to_string(bool_series, bool01) << "\n";

  std::vector<pluss::codem::Complex64> complex_vals{
      {1.25f, -0.5f}, {-2.0f, 3.0f}, {0.1f, 0.0f}, {4.0f, -1.2f}};
  let complex_series = pluss::make_series(complex_vals, {2, 2});
  pluss::PrintOptions complex_i;
  complex_i.complex_style = pluss::codem::PrintComplexStyle::CartesianI;
  std::cout << "complex cartesian i-style:\n" << pluss::print_series(complex_series, complex_i) << "\n";

  // 8) Optional ANSI coloring
  pluss::PrintOptions colored;
  colored.enable_color = true;
  colored.pretty_cell_max_width = 10;
  std::cout << "colorized (if terminal supports ANSI):\n" << pluss::print_series(rand_vals, colored) << "\n";

  // 9) Constrained let-like declarations (`let_as`)
  let_as(int, float) price = 25;
  price = 19.5f;
  std::cout << "price one-of<int,float>: ";
  price.visit([](const auto& v) { std::cout << v; });
  std::cout << "\n";

  let_as(pluss::Series, double, std::string) content = std::string("headline");
  content = 3.14159;
  std::cout << "content one-of<Series,double,string>: ";
  content.visit([](const auto& v) {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, pluss::Series>) {
      std::cout << "Series(numel=" << v.numel() << ")";
    } else {
      std::cout << v;
    }
  });
  std::cout << "\n";

  return 0;
}
