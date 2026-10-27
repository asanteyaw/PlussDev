#include <pluss/pluss.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <string>

namespace {

using pluss::Series;

void heading(const char *label) { std::cout << "=== " << label << " ===\n"; }

void show(const char *label, const Series &value) {
  std::cout << label << " shape=[";
  const auto shape = value.sizes();
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (i != 0)
      std::cout << ',';
    std::cout << shape[i];
  }
  std::cout << "] values=[";
  const auto values = value.to(pluss::pCPU, pluss::ScalarType::Float64)
                          .host<double>();
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0)
      std::cout << ',';
    std::cout << values[i];
  }
  std::cout << "]\n";
}

void show_error(const char *label, const auto &operation) {
  try {
    operation();
  } catch (const std::exception &error) {
    std::string message = error.what();
    if (const auto newline = message.find('\n'); newline != std::string::npos)
      message.resize(newline);
    std::cout << label << '=' << message << '\n';
  }
}

} // namespace

int main() {
  std::cout << std::setprecision(10);

  heading("calc_entry");
  const auto scalar = pluss::make_series(0.5);
  const auto sine = [](const Series &x) { return x.sin(); };
  show("dot", scalar.calc.diff(sine));
  show("call", scalar.calc().diff(sine));
  Series::CalcAccessor::CalcOptions second_options;
  second_options.order = 2;
  const auto second = scalar.calc(second_options);
  show("bound", second.diff(sine));
  std::cout << "bound_order=" << second.options().order << "\n\n";

  heading("diff_orders");
  const auto composite = [](const Series &x) { return x.log().mul(x.asin()); };
  show("value", scalar.calc.diff(composite, 0));
  show("first", scalar.calc.diff(composite, 1));
  show("fourth", scalar.calc.diff(composite, 4));
  std::cout << '\n';

  heading("diff_tensor_and_elementwise");
  const auto vector = pluss::make_series({0.5, 1.0, 1.5});
  show("full_first", vector.calc.diff(sine));
  Series::CalcAccessor::CalcOptions elementwise;
  elementwise.elementwise = true;
  show("elementwise_first", vector.calc(elementwise).diff(sine));
  show("elementwise_fourth", vector.calc(elementwise).diff(sine, 4));
  std::cout << '\n';

  heading("diff_higher_rank");
  const auto matrix = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  show("matrix_elementwise_second",
       matrix.calc(elementwise).diff(
           [](const Series &x) { return x.pow(3.0); }, 2));
  std::cout << '\n';

  heading("diff_dependency_failures");
  show("constant_default",
       vector.calc(elementwise).diff(
           [](const Series &x) { return x.mul(0.0).add(7.0).detach(); }));
  Series::CalcAccessor::CalcOptions strict = elementwise;
  strict.strict_dependency = true;
  show_error("constant_strict", [&] {
    (void)vector.calc(strict).diff(
        [](const Series &x) { return x.mul(0.0).add(7.0).detach(); });
  });
  show_error("elementwise_shape", [&] {
    (void)vector.calc(elementwise).diff(
        [](const Series &x) { return x.sum(); });
  });
  show_error("negative_order", [&] { (void)scalar.calc.diff(sine, -1); });
  std::cout << '\n';

  heading("grad");
  const auto point = pluss::make_series({2.0, 3.0});
  show("quadratic",
       point.calc.grad([](const Series &x) { return x.square().sum(); }));
  const auto grid = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  show("matrix_objective", grid.calc.grad([](const Series &x) {
    return x.square().mul(0.5).sum();
  }));
  show_error("nonscalar", [&] {
    (void)point.calc.grad([](const Series &x) { return x.square(); });
  });
  std::cout << '\n';

  heading("jac");
  show("coupled", point.calc.jac([](const Series &x) {
    return x.square().unite({x.sum().reshape({1})}, 0);
  }));
  show("matrix_to_matrix",
       grid.calc.jac([](const Series &x) { return x.transpose(0, 1); }));
  std::cout << '\n';

  heading("hess");
  show("cubic_sum",
       point.calc.hess([](const Series &x) { return x.pow(3.0).sum(); }));
  show("mixed_terms", point.calc.hess([](const Series &x) {
    const auto a = x.select(0, 0);
    const auto b = x.select(0, 1);
    return a.square().mul(b).add(b.pow(3.0));
  }));
  show_error("nonscalar", [&] {
    (void)point.calc.hess([](const Series &x) { return x.square(); });
  });
  std::cout << '\n';

  heading("antidiff_endpoints");
  Series::CalcAccessor::CalcOptions fixed;
  fixed.intervals = 32;
  fixed.adaptive = false;
  const auto endpoints = pluss::make_series({{0.5, 1.0}, {1.5, 2.0}});
  show("square_from_zero", endpoints.calc.antidiff(
                               [](const Series &t) { return t.square(); }, 0.0,
                               fixed));
  const auto reverse_endpoint = pluss::make_series(-1.0);
  show("reverse_bound", reverse_endpoint.calc.antidiff(
                            [](const Series &t) { return t; }, 0.0, fixed));
  std::cout << '\n';

  heading("antidiff_repeated");
  Series::CalcAccessor::CalcOptions repeated = fixed;
  repeated.order = 2;
  repeated.initial = {2.0, 3.0};
  show("second_integral", endpoints.calc.antidiff(
                              [](const Series &t) { return t.mul(0.0).add(1.0); },
                              0.0, repeated));
  repeated.initial.clear();
  show("zero_initial", endpoints.calc.antidiff(
                           [](const Series &t) { return t.mul(0.0).add(1.0); },
                           0.0, repeated));
  std::cout << '\n';

  heading("antidiff_rules");
  Series::CalcAccessor::CalcOptions trapezoid = fixed;
  trapezoid.rule = Series::CalcAccessor::Rule::Trapezoid;
  trapezoid.intervals = 8;
  Series::CalcAccessor::CalcOptions simpson = fixed;
  simpson.intervals = 8;
  const auto one = pluss::make_series(1.0);
  show("trapezoid_x4", one.calc.antidiff(
                           [](const Series &t) { return t.pow(4.0); }, 0.0,
                           trapezoid));
  show("simpson_x4", one.calc.antidiff(
                         [](const Series &t) { return t.pow(4.0); }, 0.0,
                         simpson));
  Series::CalcAccessor::CalcOptions adaptive;
  adaptive.intervals = 8;
  show("adaptive_sine",
       pluss::make_series(std::numbers::pi)
           .calc.antidiff([](const Series &t) { return t.sin(); }, 0.0,
                          adaptive));
  std::cout << '\n';

  heading("antidiff_bound_options");
  Series::CalcAccessor::CalcOptions bound;
  bound.lower = 0.0;
  bound.intervals = 32;
  bound.adaptive = false;
  const auto configured = endpoints.calc(bound);
  show("bound_lower", configured.antidiff(
                          [](const Series &t) { return t.square(); }));
  show("explicit_override", configured.antidiff(
                                [](const Series &t) { return t.square(); }, 1.0));
  std::cout << "lower=" << *configured.options().lower
            << " intervals=" << configured.options().intervals
            << " adaptive=" << std::boolalpha
            << configured.options().adaptive << "\n\n";

  heading("antidiff_endpoint_sensitivity");
  auto upper = pluss::make_series(0.75);
  upper.tracks_slope_(true);
  const auto area = upper.calc.antidiff(
      [](const Series &t) { return t.square(); }, 0.0, fixed);
  area.reverse();
  show("area", area);
  show("darea_dupper", upper.sensitivity());
  std::cout << '\n';

  heading("antidiff_failures");
  show_error("missing_lower", [&] {
    (void)one.calc.antidiff([](const Series &t) { return t; });
  });
  show_error("bad_shape", [&] {
    (void)one.calc.antidiff(
        [](const Series &t) { return t.sum(); }, 0.0, fixed);
  });
  show_error("singular", [&] {
    (void)one.calc.antidiff(
        [](const Series &t) { return t.abs().pow(-0.5); }, -1.0, fixed);
  });
  Series::CalcAccessor::CalcOptions bad_initial = fixed;
  bad_initial.order = 2;
  bad_initial.initial = {1.0};
  show_error("bad_initial", [&] {
    (void)one.calc.antidiff([](const Series &t) { return t; }, 0.0,
                            bad_initial);
  });
  Series::CalcAccessor::CalcOptions bad_rule = fixed;
  bad_rule.rule = static_cast<Series::CalcAccessor::Rule>(255);
  show_error("bad_rule", [&] {
    (void)one.calc.antidiff([](const Series &t) { return t; }, 0.0, bad_rule);
  });
}
