#include "pluss/ai/ice/optim.h"

namespace pluss::ai::ice {

std::unique_ptr<pluss::nn::optim::Optimizer> make_optimizer(
    const std::vector<Series*>& parameters, const TrainOptions& options) {
  namespace optim = pluss::nn::optim;
  PLUSS_CHECK(!parameters.empty(), "pluss::ai::ice::make_optimizer: parameters must not be empty.");
  const auto params_view = pluss::msharp::RangeView::Of<Series*>(parameters);

  std::unique_ptr<optim::Optimizer> optimizer;
  switch (options.optimizer) {
    case OptimizerKind::Adam: {
      optim::AdamOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::Adam>(params_view, opt);
      break;
    }
    case OptimizerKind::AdamW: {
      optim::AdamWOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::AdamW>(params_view, opt);
      break;
    }
    case OptimizerKind::SGD: {
      optim::SGDOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::SGD>(params_view, opt);
      break;
    }
    case OptimizerKind::RMSProp: {
      optim::RMSpropOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::RMSprop>(params_view, opt);
      break;
    }
    case OptimizerKind::Adagrad: {
      optim::AdagradOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::Adagrad>(params_view, opt);
      break;
    }
    case OptimizerKind::NAdam: {
      optim::NAdamOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.clip = options.grad_clip;
      optimizer = std::make_unique<optim::NAdam>(params_view, opt);
      break;
    }
  }
  PLUSS_CHECK(optimizer != nullptr, "pluss::ai::ice::make_optimizer: unknown OptimizerKind.");

  optim::StepDiagnosticsOptions diagnostics{};
  diagnostics.fail_if_non_finite_sensitivity = options.fail_on_non_finite;
  diagnostics.fail_if_non_finite_parameter = options.fail_on_non_finite;
  diagnostics.fail_if_non_finite_loss = options.fail_on_non_finite;
  optimizer->set_step_diagnostics(diagnostics);

  return optimizer;
}

} // namespace pluss::ai::ice
