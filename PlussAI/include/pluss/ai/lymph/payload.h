#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/time.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

// payload.h -- typed Workflow input carrier.
//
// Role: keep frame/batch data and side-channel ML roles (labels, weights,
// groups, time keys) in one value object without Python-style keyword routing
// or adding a large method surface to Workflow. Construction is by the
// lymph::workflow::payload::make(...) factory; after existence, roles are attached with
// short methods on Payload itself.

namespace pluss::ai::lymph::workflow::payload {

class Payload final {
 public:
  Payload() = default;

  [[nodiscard]] bool has_frame() const noexcept { return frame_.has_value(); }
  [[nodiscard]] bool has_features() const noexcept { return features_.has_value(); }
  [[nodiscard]] bool has_labeled() const noexcept { return labeled_.has_value(); }
  [[nodiscard]] bool has_label() const noexcept { return label_.defined(); }
  [[nodiscard]] bool has_weights() const noexcept {
    return weights_.defined() || options_.sample_weight_feature.has_value() ||
           (labeled_.has_value() && labeled_->has_weights());
  }
  [[nodiscard]] bool has_groups() const noexcept { return groups_.defined(); }
  [[nodiscard]] bool has_time() const noexcept {
    return time_.defined() || !time_feature_.empty() || time_options_.has_value();
  }
  [[nodiscard]] bool has_label_feature() const noexcept { return options_.label_feature.has_value(); }
  [[nodiscard]] bool has_weight_feature() const noexcept { return options_.sample_weight_feature.has_value(); }
  [[nodiscard]] bool has_external_weights() const noexcept { return weights_.defined(); }

  [[nodiscard]] const DataFrameBatchOptions& data() const noexcept { return options_; }
  [[nodiscard]] DataFrameBatchOptions& data() noexcept { return options_; }

  [[nodiscard]] const pluss::table::DataFrame& frame() const {
    if (!frame_.has_value()) {
      ::pluss::throw_error("lymph::workflow::Payload::frame: payload does not contain a DataFrame.");
    }
    return *frame_;
  }

  [[nodiscard]] const FeatureBatch& features() const {
    if (!features_.has_value()) {
      ::pluss::throw_error("lymph::workflow::Payload::features: payload does not contain a FeatureBatch.");
    }
    return *features_;
  }

  [[nodiscard]] const LabeledBatch& labeled() const {
    if (!labeled_.has_value()) {
      ::pluss::throw_error("lymph::workflow::Payload::labeled: payload does not contain a LabeledBatch.");
    }
    return *labeled_;
  }

  [[nodiscard]] const Series& label() const noexcept { return label_; }
  [[nodiscard]] const Series& weights() const noexcept { return weights_; }
  [[nodiscard]] const Series& groups() const noexcept { return groups_; }
  [[nodiscard]] const Series& time() const noexcept { return time_; }
  [[nodiscard]] std::string time_feature() const {
    if (!time_feature_.empty()) return time_feature_;
    if (time_options_.has_value()) return time_options_->index;
    return {};
  }
  [[nodiscard]] const std::optional<TimeOptions>& time_options() const noexcept { return time_options_; }

  Payload& features(std::vector<std::string> names) {
    options_.feature_names = std::move(names);
    return *this;
  }

  Payload& label(std::string feature_name) {
    // Supervision is one role with two spellings -- attaching it by column
    // name clears any previously attached Series label (and vice versa), so
    // the later .label(...) call always wins and has_label() /
    // has_label_feature() stay mutually exclusive.
    options_.label_feature = std::move(feature_name);
    label_ = pluss::series::undefined();
    return *this;
  }

  Payload& label(Series y) {
    label_ = std::move(y);
    options_.label_feature.reset();
    return *this;
  }

  Payload& weights(std::string feature_name) {
    options_.sample_weight_feature = std::move(feature_name);
    return *this;
  }

  Payload& weights(Series sample_weights) {
    weights_ = std::move(sample_weights);
    return *this;
  }

  Payload& groups(Series groups) {
    groups_ = std::move(groups);
    return *this;
  }

  Payload& time(std::string feature_name) {
    if (feature_name.empty()) {
      ::pluss::throw_error("lymph::workflow::Payload::time: time feature name cannot be empty.");
    }
    time_feature_ = std::move(feature_name);
    return *this;
  }

  Payload& time(TimeOptions options) {
    validate_time_options(options, "lymph::workflow::Payload::time");
    time_feature_ = options.index;
    time_options_ = std::move(options);
    return *this;
  }

  Payload& time(Series values) {
    if (!values.defined()) {
      ::pluss::throw_error("lymph::workflow::Payload::time: time Series must be defined.");
    }
    time_ = std::move(values);
    return *this;
  }

  Payload& layout(ObservationLayout layout) noexcept {
    options_.layout = layout;
    return *this;
  }

  Payload& note(DataNotePolicy policy) noexcept {
    options_.note_policy = policy;
    return *this;
  }

  Payload& thread(DataThreadPolicy policy) noexcept {
    options_.thread_policy = policy;
    return *this;
  }

  [[nodiscard]] Payload with_frame(pluss::table::DataFrame frame) const {
    Payload out(*this);
    out.frame_ = std::move(frame);
    return out;
  }

  [[nodiscard]] FeatureBatch feature_batch() const {
    if (features_.has_value()) return *features_;
    if (labeled_.has_value()) return labeled_->features;
    if (frame_.has_value()) {
      return resolve(feature_batch_from_dataframe(*frame_, options_), "lymph::workflow::Payload::feature_batch");
    }
    ::pluss::throw_error("lymph::workflow::Payload::feature_batch: payload has no frame or feature batch.");
  }

  [[nodiscard]] LabeledBatch labeled_batch() const {
    LabeledBatch out{};
    if (labeled_.has_value()) {
      out = *labeled_;
      if (weights_.defined() && !out.has_weights()) out.sample_weights = weights_;
      resolve(validate_labeled(out), "lymph::workflow::Payload::labeled_batch");
      return out;
    }
    if (frame_.has_value() && options_.label_feature.has_value()) {
      out = resolve(labeled_batch_from_dataframe(*frame_, options_), "lymph::workflow::Payload::labeled_batch");
      if (weights_.defined() && !out.has_weights()) out.sample_weights = weights_;
      resolve(validate_labeled(out), "lymph::workflow::Payload::labeled_batch");
      return out;
    }
    if (label_.defined()) {
      out.features = feature_batch();
      out.y = label_;
      out.sample_weights = weights_;
      resolve(validate_labeled(out), "lymph::workflow::Payload::labeled_batch");
      return out;
    }
    ::pluss::throw_error("lymph::workflow::Payload::labeled_batch: payload has no labels.");
  }

 private:
  explicit Payload(pluss::table::DataFrame frame, DataFrameBatchOptions options)
      : frame_(std::move(frame)), options_(std::move(options)) {}

  explicit Payload(FeatureBatch features)
      : features_(std::move(features)) {}

  explicit Payload(LabeledBatch labeled)
      : labeled_(std::move(labeled)) {}

  std::optional<pluss::table::DataFrame> frame_{};
  std::optional<FeatureBatch> features_{};
  std::optional<LabeledBatch> labeled_{};
  DataFrameBatchOptions options_{};
  Series label_ = pluss::series::undefined();
  Series weights_ = pluss::series::undefined();
  Series groups_ = pluss::series::undefined();
  Series time_ = pluss::series::undefined();
  std::string time_feature_{};
  std::optional<TimeOptions> time_options_{};

  friend Payload payload_make_(pluss::table::DataFrame frame, DataFrameBatchOptions options);
  friend Payload payload_make_(FeatureBatch features);
  friend Payload payload_make_(LabeledBatch labeled);
};
} // namespace pluss::ai::lymph::workflow::payload

namespace pluss::ai::lymph::workflow {

using Payload = payload::Payload;

namespace payload {

[[nodiscard]] inline Payload payload_make_(pluss::table::DataFrame frame, DataFrameBatchOptions options) {
  return Payload(std::move(frame), std::move(options));
}

[[nodiscard]] inline Payload payload_make_(FeatureBatch features) {
  return Payload(std::move(features));
}

[[nodiscard]] inline Payload payload_make_(LabeledBatch labeled) {
  return Payload(std::move(labeled));
}

[[nodiscard]] inline Payload make(pluss::table::DataFrame frame, DataFrameBatchOptions options = {}) {
  return payload_make_(std::move(frame), std::move(options));
}

[[nodiscard]] inline Payload make(FeatureBatch features) {
  return payload_make_(std::move(features));
}

[[nodiscard]] inline Payload make(LabeledBatch labeled) {
  return payload_make_(std::move(labeled));
}

} // namespace payload

} // namespace pluss::ai::lymph::workflow
