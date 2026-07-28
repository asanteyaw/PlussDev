#include "pluss/ai/ice/optim.h"

namespace pluss::ai::ice {

std::unique_ptr<pluss::nn::optim::Optimizer> make_optimizer(
    const std::vector<Series*>& parameters, const TrainOptions& options) {
  namespace optim = pluss::nn::optim;
  PLUSS_CHECK(!parameters.empty(), "pluss::ai::ice::make_optimizer: parameters must not be empty.");
  PLUSS_CHECK(options.objective_regularization.weight_decay >= 0.0,
              "pluss::ai::ice::make_optimizer: weight_decay must be non-negative.");
  PLUSS_CHECK(options.objective_regularization.l1_penalty >= 0.0 &&
                  options.objective_regularization.l2_penalty >= 0.0,
              "pluss::ai::ice::make_optimizer: L1/L2 penalties must be non-negative.");
  PLUSS_CHECK(!options.objective_regularization.decoupled_weight_decay ||
                  options.optimizer == OptimizerKind::AdamW ||
                  options.optimizer == OptimizerKind::NAdam,
              "pluss::ai::ice::make_optimizer: decoupled_weight_decay is only supported for "
              "AdamW and NAdam.");
  const auto params_view = pluss::msharp::RangeView::Of<Series*>(parameters);

  std::unique_ptr<optim::Optimizer> optimizer;
  switch (options.optimizer) {
    case OptimizerKind::Adam: {
      optim::AdamOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::Adam>(params_view, opt);
      break;
    }
    case OptimizerKind::AdamW: {
      optim::AdamWOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::AdamW>(params_view, opt);
      break;
    }
    case OptimizerKind::SGD: {
      optim::SGDOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::SGD>(params_view, opt);
      break;
    }
    case OptimizerKind::RMSProp: {
      optim::RMSpropOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::RMSprop>(params_view, opt);
      break;
    }
    case OptimizerKind::Adagrad: {
      optim::AdagradOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::Adagrad>(params_view, opt);
      break;
    }
    case OptimizerKind::NAdam: {
      optim::NAdamOptions opt{};
      opt.learning_rate = options.learning_rate;
      opt.weight_decay = options.objective_regularization.weight_decay;
      opt.decoupled_weight_decay = options.objective_regularization.decoupled_weight_decay;
      opt.clip = options.execution.grad_clip;
      optimizer = std::make_unique<optim::NAdam>(params_view, opt);
      break;
    }
  }
  PLUSS_CHECK(optimizer != nullptr, "pluss::ai::ice::make_optimizer: unknown OptimizerKind.");

  optim::StepDiagnosticsOptions diagnostics{};
  diagnostics.fail_if_non_finite_sensitivity = options.execution.fail_on_non_finite;
  diagnostics.fail_if_non_finite_parameter = options.execution.fail_on_non_finite;
  diagnostics.fail_if_non_finite_loss = options.execution.fail_on_non_finite;
  optimizer->set_step_diagnostics(diagnostics);

  return optimizer;
}

} // namespace pluss::ai::ice
