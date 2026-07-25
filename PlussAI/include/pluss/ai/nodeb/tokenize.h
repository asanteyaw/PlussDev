#pragma once

#include "pluss/ai/nodeb/workflow.h"

namespace pluss::ai::nodeb {

class RuleTokenizerStage final : public IStage {
public:
  explicit RuleTokenizerStage(RuleTokenizerOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

private:
  RuleTokenizerOptions options_{};
};

namespace stage {

[[nodiscard]] Stage tokenize(RuleTokenizerOptions options = {});

} // namespace stage

} // namespace pluss::ai::nodeb
