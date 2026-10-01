#pragma once

#include "pluss/ai/ice/options.h"
#include "pluss/nn.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::ai::ice {

struct BatchField {
  std::string name;
  Series values = pluss::series::undefined();
};

namespace batch_field_names {

inline constexpr std::string_view inputs = "inputs";
inline constexpr std::string_view static_inputs = "static_inputs";
inline constexpr std::string_view encoder_inputs = "encoder_inputs";
inline constexpr std::string_view decoder_inputs = "decoder_inputs";
inline constexpr std::string_view encoder_target = "encoder_target";

} // namespace batch_field_names

class Batch {
public:
  [[nodiscard]] bool has(std::string_view name) const noexcept;
  [[nodiscard]] const Series& field(std::string_view name) const;
  [[nodiscard]] Series& field(std::string_view name);
  [[nodiscard]] const std::vector<BatchField>& fields() const noexcept { return fields_; }

  void set(std::string name, Series values);

  [[nodiscard]] bool has_inputs() const noexcept { return has(batch_field_names::inputs); }
  [[nodiscard]] const Series& inputs() const { return field(batch_field_names::inputs); }
  [[nodiscard]] Series& inputs() { return field(batch_field_names::inputs); }
  void set_inputs(Series values) { set(std::string(batch_field_names::inputs), std::move(values)); }

  [[nodiscard]] bool has_targets() const noexcept { return targets_.defined(); }
  [[nodiscard]] const Series& targets() const noexcept { return targets_; }
  [[nodiscard]] Series& targets() noexcept { return targets_; }
  void set_targets(Series values) { targets_ = std::move(values); }

private:
  std::vector<BatchField> fields_{};
  Series targets_ = pluss::series::undefined();
};

enum class StepStage : std::uint8_t {
  Learn = 0,
  Valid = 1,
  Test = 2,
  Infer = 3,
};

struct StepContext {
  const TrainOptions* recipe = nullptr;
  std::optional<LossKind> loss_kind = std::nullopt;
  StepStage stage = StepStage::Infer;
  std::int64_t epoch = -1;
  std::int64_t batch_index = -1;
};

struct StepResult {
  Series predictions = pluss::series::undefined();
  Series loss = pluss::series::undefined();

  [[nodiscard]] bool has_predictions() const noexcept { return predictions.defined(); }
  [[nodiscard]] bool has_loss() const noexcept { return loss.defined(); }
};

inline bool Batch::has(std::string_view name) const noexcept {
  for (const auto& field : fields_) {
    if (field.name == name) return true;
  }
  return false;
}

inline const Series& Batch::field(std::string_view name) const {
  for (const auto& field : fields_) {
    if (field.name == name) return field.values;
  }
  PLUSS_FAIL("pluss::ai::ice::Batch::field: no field named '" + std::string(name) + "'.");
}

inline Series& Batch::field(std::string_view name) {
  for (auto& field : fields_) {
    if (field.name == name) return field.values;
  }
  PLUSS_FAIL("pluss::ai::ice::Batch::field: no field named '" + std::string(name) + "'.");
}

inline void Batch::set(std::string name, Series values) {
  for (auto& field : fields_) {
    if (field.name == name) {
      field.values = std::move(values);
      return;
    }
  }
  fields_.push_back(BatchField{
      .name = std::move(name),
      .values = std::move(values),
  });
}

} // namespace pluss::ai::ice
