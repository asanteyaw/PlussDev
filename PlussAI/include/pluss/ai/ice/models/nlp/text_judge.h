#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/ai/ice/platform.h"
#include "pluss/ai/nodeb/workflow.h"
#include "pluss/msharp/msharp.h"
#include "pluss/pluss.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pluss::ai::ice::models {

struct TextJudgeOptions {
  std::int64_t width = 64;
  std::int64_t num_layers = 2;
  std::int64_t num_heads = 4;
  std::int64_t feedforward_width = 128;
  double dropout = 0.1;
  std::int64_t max_positions = 4096;
  bool use_lemmas = true;
  bool skip_punct = true;
  bool lowercase = true;
  Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  std::uint64_t seed = 0;
};

struct TextJudgeTrainOptions {
  std::int64_t batch_size = 32;
  double learning_rate = 1e-3;
  OptimizerKind optimizer = OptimizerKind::Adam;
  bool shuffle = true;
  std::uint64_t seed = 0;
  ValidationPolicy validation{};
  ArchitectureRegularization architecture_regularization{};
  ObjectiveRegularization objective_regularization{};
  LoopRegularization loop_regularization{};
  ExecutionPolicy execution{};
};

struct TextJudgePrediction {
  std::string label{};
  double confidence = 0.0;
};

struct TextJudgeReport {
  double loss = 0.0;
  double accuracy = 0.0;
  std::int64_t count = 0;
};

class TextJudge final {
public:
  struct Impl;

  explicit TextJudge(TextJudgeOptions options = {});

  void learn(const std::vector<pluss::ai::nodeb::Doc>& docs,
             pluss::msharp::RangeView::Of<std::string_view> labels,
             const TextJudgeTrainOptions& options = {});
  void learn(pluss::msharp::RangeView::Of<std::string_view> texts,
             pluss::msharp::RangeView::Of<std::string_view> labels,
             const TextJudgeTrainOptions& options = {});
  void learn(const std::vector<std::string>& texts,
             const std::vector<std::string>& labels,
             const TextJudgeTrainOptions& options = {});
  void learn(const pluss::Note& texts,
             const pluss::Note& labels,
             const TextJudgeTrainOptions& options = {});

  [[nodiscard]] std::vector<TextJudgePrediction> predict(
      const std::vector<pluss::ai::nodeb::Doc>& docs) const;
  [[nodiscard]] std::vector<TextJudgePrediction> predict(
      pluss::msharp::RangeView::Of<std::string_view> texts) const;
  [[nodiscard]] std::vector<TextJudgePrediction> predict(
      const std::vector<std::string>& texts) const;
  [[nodiscard]] std::vector<TextJudgePrediction> predict(const pluss::Note& texts) const;

  [[nodiscard]] TextJudgeReport assess(const std::vector<pluss::ai::nodeb::Doc>& docs,
                                       pluss::msharp::RangeView::Of<std::string_view> labels) const;
  [[nodiscard]] TextJudgeReport assess(pluss::msharp::RangeView::Of<std::string_view> texts,
                                       pluss::msharp::RangeView::Of<std::string_view> labels) const;
  [[nodiscard]] TextJudgeReport assess(const std::vector<std::string>& texts,
                                       const std::vector<std::string>& labels) const;
  [[nodiscard]] TextJudgeReport assess(const pluss::Note& texts,
                                       const pluss::Note& labels) const;

  void save(std::string_view filepath) const;
  [[nodiscard]] static TextJudge load(std::string_view filepath);

  [[nodiscard]] const std::vector<std::string>& labels() const noexcept;
  [[nodiscard]] bool ready() const noexcept;
  TextJudge& to(Platform platform);

private:
  std::shared_ptr<Impl> impl_;
  TextJudgeOptions options_{};
};

} // namespace pluss::ai::ice::models
