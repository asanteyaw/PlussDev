#pragma once

#include "pluss/ai/ice/callback.h"
#include "pluss/ai/ice/source.h"
#include "pluss/ai/ice/step.h"
#include "pluss/nn.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::table {
class DataFrame;
} // namespace pluss::table

namespace pluss::ai::ice {

class Model;

// Shared trainer/execution surface over ice::Model. Model keeps the
// one-liner convenience methods, but Coach owns the central learn/infer/
// assess orchestration so future callbacks, checkpoint policy, execution
// modes, and regularization logic have one place to plug in.
class Coach {
public:
  explicit Coach(TrainOptions options = {},
                 std::vector<std::shared_ptr<Callback>> callbacks = {})
      : options_(std::move(options)),
        callbacks_(std::move(callbacks)) {}

  [[nodiscard]] const TrainOptions& options() const noexcept { return options_; }
  [[nodiscard]] const std::vector<std::shared_ptr<Callback>>& callbacks() const noexcept { return callbacks_; }
  Coach& add_callback(std::shared_ptr<Callback> callback);

  void learn(Model& model, Source& source) const;
  void learn(Model& model, const Series& x_train, const Series& y_train) const;
  void learn(Model& model,
             const Series& x_train,
             const Series& y_train,
             const Series& x_val,
             const Series& y_val) const;
  void learn(Model& model, const pluss::table::DataFrame& df, std::string_view target) const;

  [[nodiscard]] Series infer(const Model& model, Source& source) const;
  [[nodiscard]] Series infer(const Model& model, const Series& x, std::int64_t chunk_size = 0) const;
  [[nodiscard]] Series infer(const Model& model,
                             const pluss::table::DataFrame& df,
                             std::string_view target = {},
                             std::int64_t chunk_size = 0) const;

  [[nodiscard]] AssessReport assess(const Model& model, Source& source, LossKind loss) const;
  [[nodiscard]] AssessReport assess(const Model& model,
                                    const Series& x,
                                    const Series& y,
                                    LossKind loss) const;
  [[nodiscard]] AssessReport assess(const Model& model,
                                    const pluss::table::DataFrame& df,
                                    std::string_view target,
                                    LossKind loss) const;

private:
  [[nodiscard]] CallbackChain build_train_callbacks_(const TrainOptions& options, bool has_validation) const;
  void train_loop_(Model& model, Source& source, const TrainOptions& options) const;
  [[nodiscard]] AssessReport assess_source_(const Model& model,
                                            Source& source,
                                            LossKind loss_kind) const;
  [[nodiscard]] Series infer_source_(const Model& model, Source& source) const;

  TrainOptions options_{};
  std::vector<std::shared_ptr<Callback>> callbacks_{};
};

} // namespace pluss::ai::ice
