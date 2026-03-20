#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::subsp::math::diff::Trace;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  std::vector<Series> p{
      pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg),
      pluss::make_series(std::vector<double>{-1.0}, {1}, cfg),
  };
  std::vector<Series> p_tape{
      pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg),
      pluss::make_series(std::vector<double>{-1.0}, {1}, cfg),
  };
  const Series wt = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);
  const Series bt = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

  pluss::optim::MinimaxOptions opts{};
  opts.adam.learning_rate = 0.03;
  opts.adam.weight_decay = 1e-3;
  opts.dual_learning_rate = 0.08;
  opts.equality_penalty = 8.0;
  opts.inequality_penalty = 8.0;

  pluss::optim::MinimaxConstraints c{};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() + x[1] - 2.0}; };
  c.equalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(x[0]) + x[1] - 2.0};
  };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 5.0}; };
  c.inequalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(x[0])) - 5.0};
  };
  c.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg), Series::undefined()};

  auto st = pluss::optim::minimax_state_like(p, c);
  auto st_tape = pluss::optim::minimax_state_like(p_tape, c);

  for (int i = 0; i < 200; ++i) {
    pluss::optim::minimax_step_inplace(p, std::vector<Series>{p[0] - wt, p[1] - bt}, st, opts, c);
  }

  for (int i = 0; i < 200; ++i) {
    auto tw = pluss::subsp::math::diff::trace(p_tape[0], true);
    auto tb = pluss::subsp::math::diff::trace(p_tape[1], true);
    const Series loss = ((tw.value() - wt).square().sum() + (tb.value() - bt).square().sum()) * 0.5;
    tw.clear_sensitivity();
    tb.clear_sensitivity();
    loss.reverse();
    p_tape[0] = tw.value();
    p_tape[1] = tb.value();
    pluss::optim::minimax_step_inplace(p_tape, st_tape, opts, c);
  }

  std::cout << "explicit w=[" << p[0].select(0, 0).item<double>() << ", " << p[0].select(0, 1).item<double>()
            << "] b=" << p[1].item<double>() << '\n';
  std::cout << "tape w=[" << p_tape[0].select(0, 0).item<double>() << ", " << p_tape[0].select(0, 1).item<double>()
            << "] b=" << p_tape[1].item<double>() << '\n';
  return 0;
}
