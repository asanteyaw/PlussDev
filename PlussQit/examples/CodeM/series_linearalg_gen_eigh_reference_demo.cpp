#include <pluss/pluss.h>

#include <complex>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace {

void heading(const char *label) { std::cout << "=== " << label << " ===\n"; }

void values(const char *label, const pluss::Series &value) {
  std::cout << label << "=[";
  const auto lanes = value.to_vec<double>();
  for (std::size_t index = 0; index < lanes.size(); ++index) {
    if (index)
      std::cout << ", ";
    std::cout << std::fixed << std::setprecision(10) << lanes[index];
  }
  std::cout << "] shape=[";
  for (std::int64_t axis = 0; axis < value.dim(); ++axis) {
    if (axis)
      std::cout << ", ";
    std::cout << value.size(axis);
  }
  std::cout << "]\n";
}

void error(const std::exception &exception) {
  const std::string message = exception.what();
  std::cout << message.substr(0, message.find('\n')) << "\n";
}

} // namespace

int main() {
  heading("diagonal_generalized_problem");
  const auto seed = pluss::make_series({{1.0, 1.0}, {1.0, -1.0}});
  const auto a_diag = pluss::make_series({2.0, 8.0});
  const auto b_diag = pluss::make_series({1.0, 2.0});
  const auto diagonal = seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(a_diag); },
      [&](const pluss::Series &x) { return x.mul(b_diag); },
      [&](const pluss::Series &rhs) { return rhs.div(a_diag); });
  values("values", diagonal.values);
  values("residual", diagonal.residual);
  std::cout << std::boolalpha
            << "converged=" << diagonal.converged.all().item<bool>() << "\n"
            << "iterations=" << diagonal.iterations << "\n\n";

  heading("common_operator_scale");
  const auto tiny_a = a_diag.mul(1e-20);
  const auto tiny_b = b_diag.mul(1e-20);
  const auto scaled = seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(tiny_a); },
      [&](const pluss::Series &x) { return x.mul(tiny_b); },
      [&](const pluss::Series &rhs) { return rhs.div(tiny_a); });
  values("values", scaled.values);
  std::cout << "converged=" << scaled.converged.all().item<bool>() << "\n\n";

  heading("dense_operator_callbacks");
  const auto a = pluss::make_series({{4.0, 1.0}, {1.0, 3.0}});
  const auto dense = seed.linearalg().gen_eigh(
      [&](const pluss::Series &x) {
        return a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
      },
      [&](const pluss::Series &x) { return x; },
      [&](const pluss::Series &rhs) {
        return a.linearalg.lu_solve(rhs.transpose(-2, -1)).transpose(-2, -1);
      });
  values("values", dense.values);
  values("B_norm", dense.vectors.square().sum(-1, false));
  std::cout << "equation="
            << a.linearalg.mm(dense.vectors.transpose(-2, -1))
                   .transpose(-2, -1)
                   .allclose(dense.vectors.mul(dense.values.unsqueeze(-1)),
                             1e-9, 1e-9)
            << "\n\n";

  heading("one_low_mode");
  const auto one =
      pluss::make_series({{1.0, 1.0}})
          .linearalg.gen_eigh(
              [&](const pluss::Series &x) {
                return a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
              },
              [&](const pluss::Series &x) { return x; },
              [&](const pluss::Series &rhs) {
                return a.linearalg.lu_solve(rhs.transpose(-2, -1))
                    .transpose(-2, -1);
              });
  values("value", one.values);
  values("residual", one.residual);
  std::cout << "iterations=" << one.iterations << "\n\n";

  heading("batched_problem");
  const auto batch_a =
      pluss::make_series(std::vector<double>{2.0, 8.0, 8.0, 32.0}, {2, 2});
  const auto batch_seed = seed.unsqueeze(0).expand({2, 2, 2}, true);
  const auto batch = batch_seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(batch_a.unsqueeze(-2)); },
      [&](const pluss::Series &x) { return x.mul(b_diag); },
      [&](const pluss::Series &rhs) { return rhs.div(batch_a.unsqueeze(-2)); });
  values("values", batch.values);
  values("residual", batch.residual);
  std::cout << "all_converged=" << batch.converged.all().item<bool>() << "\n\n";

  heading("complex_hermitian_problem");
  using Complex = std::complex<double>;
  const auto complex_seed = pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}},
      {2, 2});
  const auto complex_a = pluss::make_series(
      std::vector<Complex>{{3.0, 0.0}, {0.0, 1.0}, {0.0, -1.0}, {3.0, 0.0}},
      {2, 2});
  const auto complex_b = pluss::make_series(
      std::vector<Complex>{{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}},
      {2, 2});
  const auto complex_result = complex_seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) {
        return complex_a.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
      },
      [&](const pluss::Series &x) {
        return complex_b.linearalg.mm(x.transpose(-2, -1)).transpose(-2, -1);
      },
      [&](const pluss::Series &rhs) {
        return complex_a.linearalg.lu_solve(rhs.transpose(-2, -1))
            .transpose(-2, -1);
      });
  values("values", complex_result.values);
  std::cout << "vectors_dtype=" << complex_result.vectors.dtype().name << "\n"
            << "converged=" << complex_result.converged.all().item<bool>()
            << "\n\n";

  heading("iteration_budget");
  const auto three_seed = pluss::make_series({{1.0, 1.0, 1.0}});
  const auto three_a = pluss::make_series({2.0, 8.0, 32.0});
  pluss::Series::LinearAlgAccessor::GenEigOptions one_step;
  one_step.max_iterations = 1;
  one_step.relative_tolerance = 1e-14;
  one_step.absolute_tolerance = 0.0;
  const auto stopped = three_seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(three_a); },
      [&](const pluss::Series &x) { return x; },
      [&](const pluss::Series &rhs) { return rhs.div(three_a); }, one_step);
  values("estimate", stopped.values);
  values("residual", stopped.residual);
  std::cout << "converged=" << stopped.converged.all().item<bool>() << "\n"
            << "iterations=" << stopped.iterations << "\n\n";

  heading("reverse_sensitivity");
  auto tracked_a = pluss::make_series({2.0, 8.0});
  tracked_a.tracks_slope_(true);
  const auto tracked = seed.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(tracked_a); },
      [&](const pluss::Series &x) { return x.mul(b_diag); },
      [&](const pluss::Series &rhs) { return rhs.div(tracked_a); });
  tracked.values.sum().reverse();
  values("values", tracked.values);
  values("d_sum_d_A_diag", tracked_a.sensitivity());
  std::cout << "has_sensitivity=" << tracked_a.has_sensitivity() << "\n\n";

  heading("float32");
  const auto seed32 = seed.to(pluss::ScalarType::Float32);
  const auto a32 = a_diag.to(pluss::ScalarType::Float32);
  const auto b32 = b_diag.to(pluss::ScalarType::Float32);
  const auto result32 = seed32.linearalg.gen_eigh(
      [&](const pluss::Series &x) { return x.mul(a32); },
      [&](const pluss::Series &x) { return x.mul(b32); },
      [&](const pluss::Series &rhs) { return rhs.div(a32); });
  std::cout << "values_dtype=" << result32.values.dtype().name << "\n"
            << "vectors_dtype=" << result32.vectors.dtype().name << "\n\n";

  heading("failures");
  try {
    (void)pluss::make_series({{1.0, 0.0}, {2.0, 0.0}})
        .linearalg.gen_eigh(
            [&](const pluss::Series &x) { return x.mul(a_diag); },
            [&](const pluss::Series &x) { return x; },
            [&](const pluss::Series &rhs) { return rhs.div(a_diag); });
  } catch (const std::exception &exception) {
    error(exception);
  }
  try {
    (void)seed.linearalg.gen_eigh(
        [&](const pluss::Series &x) { return x.select(-1, 0); },
        [&](const pluss::Series &x) { return x; },
        [&](const pluss::Series &rhs) { return rhs.div(a_diag); });
  } catch (const std::exception &exception) {
    error(exception);
  }
  try {
    const auto negative_b = pluss::make_series({-1.0, -1.0});
    (void)seed.linearalg.gen_eigh(
        [&](const pluss::Series &x) { return x.mul(a_diag); },
        [&](const pluss::Series &x) { return x.mul(negative_b); },
        [&](const pluss::Series &rhs) { return rhs.div(a_diag); });
  } catch (const std::exception &exception) {
    error(exception);
  }
}
