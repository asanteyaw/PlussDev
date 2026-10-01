#include "pluss/ai/ice/models/nlp/text_judge.h"

#include "pluss/ai/ice/io/kv_artifact.h"
#include "pluss/ai/nodeb/tasks.h"
#include "pluss/ai/nodeb/tokenize.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::ice::models {

namespace {

using nodeb::Doc;
using nodeb::Language;
using nodeb::TaskBackend;
using nodeb::TextCatOptions;
using nodeb::TextCatStage;
using nodeb::TrainOptions;
using nodeb::Workflow;

constexpr std::string_view kMagic = "pluss_ice_text_judge";
constexpr std::int64_t kSchema = 3;

[[nodiscard]] std::vector<std::string> materialize_strings_(
    pluss::msharp::RangeView::Of<std::string_view> values) {
  std::vector<std::string> out{};
  out.reserve(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    out.emplace_back(values[i]);
  }
  return out;
}

[[nodiscard]] std::vector<std::string> note_strings_(const pluss::Note& values) {
  std::vector<std::string> out{};
  out.reserve(static_cast<std::size_t>(values.numel()));
  for (std::int64_t i = 0; i < values.numel(); ++i) {
    out.emplace_back(values.at(i));
  }
  return out;
}

[[nodiscard]] Workflow text_workflow_(const TextJudgeOptions& options) {
  Workflow workflow{.lang = Language::English, .stages = {nodeb::stage::tokenize()}};
  if (options.use_lemmas) {
    workflow.append_stage(nodeb::stage::lemmatize({
        .lowercase = options.lowercase,
        .fill_from_orth_when_empty = true,
        .use_language_lookup = true,
    }));
  }
  return workflow;
}

[[nodiscard]] std::vector<Doc> infer_docs_(const TextJudgeOptions& options,
                                           pluss::msharp::RangeView::Of<std::string_view> texts) {
  return text_workflow_(options).infer(texts);
}

[[nodiscard]] std::vector<Doc> infer_docs_(const TextJudgeOptions& options,
                                           const std::vector<std::string>& texts) {
  return text_workflow_(options).infer(texts);
}

[[nodiscard]] TextCatOptions textcat_options_(const TextJudgeOptions& model_options,
                                              const TextJudgeTrainOptions& train_options,
                                              std::int64_t iterations) {
  TextCatOptions options{};
  options.lowercase = model_options.lowercase;
  options.use_lemmas = model_options.use_lemmas;
  options.skip_punct = model_options.skip_punct;
  options.backend = TaskBackend::Sequence;
  options.sequence_width = model_options.width;
  options.sequence_learning_rate = train_options.learning_rate;
  options.sequence_dropout = train_options.architecture_regularization.dropout > 0.0
                                 ? train_options.architecture_regularization.dropout
                                 : model_options.dropout;
  options.sequence_seed = model_options.seed;
  options.max_iterations = std::max<std::int64_t>(1, iterations);
  options.tolerance = std::max(1e-8, train_options.loop_regularization.min_delta);
  return options;
}

[[nodiscard]] TrainOptions nodeb_train_options_(const TextJudgeTrainOptions& options) {
  TrainOptions out{};
  out.random.seed = options.seed;
  out.random.deterministic = options.execution.deterministic;
  return out;
}

[[nodiscard]] std::string lower_ascii_(std::string_view in) {
  std::string out(in);
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return out;
}

[[nodiscard]] MonitorDirection resolve_monitor_direction_(const ValidationPolicy& validation) {
  if (validation.direction != MonitorDirection::Auto) {
    return validation.direction;
  }
  const std::string monitor = lower_ascii_(validation.monitor);
  if (monitor.find("acc") != std::string::npos || monitor.find("auc") != std::string::npos ||
      monitor.find("f1") != std::string::npos || monitor.find("precision") != std::string::npos ||
      monitor.find("recall") != std::string::npos || monitor.find("map") != std::string::npos ||
      monitor.find("ndcg") != std::string::npos || monitor.find("score") != std::string::npos) {
    return MonitorDirection::Max;
  }
  return MonitorDirection::Min;
}

[[nodiscard]] bool monitor_improved_(double current,
                                     double best,
                                     const ValidationPolicy& validation,
                                     double min_delta) {
  switch (resolve_monitor_direction_(validation)) {
    case MonitorDirection::Min: return current < best - min_delta;
    case MonitorDirection::Max: return current > best + min_delta;
    case MonitorDirection::Auto: break;
  }
  PLUSS_FAIL("pluss::ai::ice::models::TextJudge: unreachable monitor direction.");
}

using pluss::ai::ice::io::detail::collect_indexed;
using pluss::ai::ice::io::detail::parse_kv_body;
using pluss::ai::ice::io::detail::require_bool;
using pluss::ai::ice::io::detail::require_f64;
using pluss::ai::ice::io::detail::require_field;
using pluss::ai::ice::io::detail::require_i64;
using pluss::ai::ice::io::detail::require_u64;
using pluss::ai::ice::io::detail::write_bool_line;
using pluss::ai::ice::io::detail::write_kv_line;
using pluss::ai::ice::io::detail::write_num_line;

constexpr std::string_view kLoadContext = "pluss::ai::ice::models::TextJudge::load";

void validate_model_options_(const TextJudgeOptions& options) {
  PLUSS_CHECK(options.width > 0,
              "pluss::ai::ice::models::TextJudge: width must be positive.");
  PLUSS_CHECK(options.dropout >= 0.0 && options.dropout <= 1.0,
              "pluss::ai::ice::models::TextJudge: dropout must be in [0, 1].");
}

void validate_train_options_(const TextJudgeTrainOptions& options) {
  PLUSS_CHECK(options.learning_rate > 0.0 &&
                  options.execution.max_epochs > 0 &&
                  options.validation.every_n_epochs > 0 &&
                  options.validation.start_from_epoch >= 0,
              "pluss::ai::ice::models::TextJudge: learning_rate, execution.max_epochs, "
              "validation.every_n_epochs must be positive and validation.start_from_epoch "
              "must be non-negative.");
}

[[nodiscard]] std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>
split_labeled_indices_(pluss::msharp::RangeView::Of<std::string_view> labels,
                       double holdout_ratio,
                       bool shuffle,
                       std::uint64_t seed) {
  const auto count = static_cast<std::int64_t>(labels.size());
  if (count <= 1 || holdout_ratio <= 0.0) {
    std::vector<std::int64_t> all(static_cast<std::size_t>(count));
    std::iota(all.begin(), all.end(), 0);
    return {std::move(all), {}};
  }

  std::vector<std::string> label_order{};
  std::unordered_map<std::string, std::vector<std::int64_t>> groups{};
  label_order.reserve(labels.size());
  for (std::int64_t i = 0; i < count; ++i) {
    const std::string label(labels[static_cast<std::size_t>(i)]);
    if (!groups.contains(label)) {
      label_order.push_back(label);
      groups.emplace(label, std::vector<std::int64_t>{});
    }
    groups[label].push_back(i);
  }

  std::vector<std::int64_t> train_ids{};
  std::vector<std::int64_t> val_ids{};
  train_ids.reserve(static_cast<std::size_t>(count));
  val_ids.reserve(static_cast<std::size_t>(count));
  std::mt19937_64 rng(seed);
  for (const auto& label : label_order) {
    auto& ids = groups[label];
    if (shuffle) {
      std::shuffle(ids.begin(), ids.end(), rng);
    }
    const auto holdout = std::clamp<std::int64_t>(
        static_cast<std::int64_t>(std::llround(static_cast<double>(ids.size()) * holdout_ratio)),
        0,
        static_cast<std::int64_t>(ids.size()) - 1);
    const auto split = static_cast<std::size_t>(static_cast<std::int64_t>(ids.size()) - holdout);
    train_ids.insert(train_ids.end(), ids.begin(), ids.begin() + static_cast<std::ptrdiff_t>(split));
    val_ids.insert(val_ids.end(), ids.begin() + static_cast<std::ptrdiff_t>(split), ids.end());
  }
  if (shuffle) {
    std::shuffle(train_ids.begin(), train_ids.end(), rng);
    std::shuffle(val_ids.begin(), val_ids.end(), rng);
  }
  return {std::move(train_ids), std::move(val_ids)};
}

template <class T>
[[nodiscard]] std::vector<T> gather_by_index_(const std::vector<T>& values,
                                              const std::vector<std::int64_t>& indices) {
  std::vector<T> out{};
  out.reserve(indices.size());
  for (const auto idx : indices) {
    out.push_back(values[static_cast<std::size_t>(idx)]);
  }
  return out;
}

[[nodiscard]] double choose_metric_(const TextJudgeReport& report,
                                    const ValidationPolicy& validation) {
  const std::string monitor = lower_ascii_(validation.monitor);
  if (monitor.find("acc") != std::string::npos || monitor.find("score") != std::string::npos) {
    return report.accuracy;
  }
  return report.loss;
}

void validate_labeled_docs_(const std::vector<Doc>& docs,
                            pluss::msharp::RangeView::Of<std::string_view> labels) {
  PLUSS_CHECK(!docs.empty(),
              "pluss::ai::ice::models::TextJudge: expected at least one document.");
  PLUSS_CHECK(docs.size() == labels.size(),
              "pluss::ai::ice::models::TextJudge: docs and labels must have the same size.");
}

[[nodiscard]] std::vector<TextJudgePrediction> predict_impl_(TextCatStage& stage,
                                                             const std::vector<Doc>& docs,
                                                             const std::vector<std::string>& labels) {
  PLUSS_CHECK(!docs.empty(),
              "pluss::ai::ice::models::TextJudge::predict: expected at least one document.");
  std::vector<TextJudgePrediction> out{};
  out.reserve(docs.size());
  for (auto doc : docs) {
    stage.apply(doc);
    out.push_back(TextJudgePrediction{
        .label = doc.doc_label.empty() && !labels.empty() ? labels.front() : doc.doc_label,
        .confidence = std::isfinite(doc.doc_score) ? doc.doc_score : 0.0,
    });
  }
  return out;
}

[[nodiscard]] TextJudgeReport assess_impl_(TextCatStage& stage,
                                           const std::vector<Doc>& docs,
                                           pluss::msharp::RangeView::Of<std::string_view> labels,
                                           std::int64_t label_count) {
  validate_labeled_docs_(docs, labels);
  const auto preds = predict_impl_(stage, docs, {});
  std::int64_t correct = 0;
  double neg_log = 0.0;
  constexpr double eps = 1e-12;
  const double off_class_floor = 1.0 / static_cast<double>(std::max<std::int64_t>(2, label_count));
  for (std::size_t i = 0; i < preds.size(); ++i) {
    const bool hit = preds[i].label == labels[i];
    correct += hit ? 1 : 0;
    const double p = hit ? std::clamp(preds[i].confidence, eps, 1.0)
                         : std::clamp((1.0 - preds[i].confidence) * off_class_floor, eps, 1.0);
    neg_log -= std::log(p);
  }
  return TextJudgeReport{
      .loss = preds.empty() ? 0.0 : (neg_log / static_cast<double>(preds.size())),
      .accuracy = preds.empty() ? 0.0 : static_cast<double>(correct) / static_cast<double>(preds.size()),
      .count = static_cast<std::int64_t>(preds.size()),
  };
}

} // namespace

struct TextJudge::Impl {
  TextCatStage stage{TextCatOptions{.backend = TaskBackend::Sequence}};
  std::vector<std::string> label_values{};
  std::unordered_map<std::string, std::int64_t> label_to_index{};
  bool trained = false;
};

TextJudge::TextJudge(TextJudgeOptions options)
    : impl_(std::make_shared<Impl>()),
      options_(std::move(options)) {
  validate_model_options_(options_);
}

void TextJudge::learn(const std::vector<Doc>& docs,
                      pluss::msharp::RangeView::Of<std::string_view> labels,
                      const TextJudgeTrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_model_options_(options_);
  validate_train_options_(options);
  validate_labeled_docs_(docs, labels);

  auto fresh_impl = std::make_shared<Impl>();
  const auto all_labels = materialize_strings_(labels);
  for (const auto& label : all_labels) {
    if (!fresh_impl->label_to_index.contains(label)) {
      fresh_impl->label_to_index.emplace(label, static_cast<std::int64_t>(fresh_impl->label_values.size()));
      fresh_impl->label_values.push_back(label);
    }
  }
  PLUSS_CHECK(fresh_impl->label_values.size() >= 2,
              "pluss::ai::ice::models::TextJudge: expected at least two distinct labels.");

  const auto [train_ids, val_ids] = split_labeled_indices_(labels,
                                                           options.validation.split_ratio,
                                                           options.shuffle,
                                                           options.seed);
  const bool use_validation =
      options.validation.split_ratio > 0.0 && !val_ids.empty() && val_ids.size() < docs.size();
  PLUSS_CHECK(!use_validation || !train_ids.empty(),
              "pluss::ai::ice::models::TextJudge: validation.split_ratio leaves an empty training split.");
  const auto train_docs = use_validation ? gather_by_index_(docs, train_ids) : docs;
  const auto train_labels = use_validation ? gather_by_index_(all_labels, train_ids) : all_labels;
  const auto val_docs = use_validation ? gather_by_index_(docs, val_ids) : std::vector<Doc>{};
  const auto val_labels = use_validation ? gather_by_index_(all_labels, val_ids) : std::vector<std::string>{};

  auto train_stage = [&](std::int64_t iterations) {
    TextCatStage stage(textcat_options_(options_, options, iterations));
    std::vector<std::string_view> train_label_views{};
    train_label_views.reserve(train_labels.size());
    for (const auto& label : train_labels) train_label_views.push_back(label);
    stage.learn(train_docs,
                pluss::msharp::RangeView::Of<std::string_view>(train_label_views),
                nodeb_train_options_(options));
    return stage;
  };

  if (!use_validation) {
    fresh_impl->stage = train_stage(options.execution.max_epochs);
    fresh_impl->trained = true;
    impl_ = std::move(fresh_impl);
    return;
  }

  std::optional<std::vector<std::string>> best_snapshot = std::nullopt;
  double best_metric = resolve_monitor_direction_(options.validation) == MonitorDirection::Min
                           ? std::numeric_limits<double>::infinity()
                           : -std::numeric_limits<double>::infinity();
  std::int64_t stale = 0;
  TextCatStage last_stage(textcat_options_(options_, options, 1));
  bool have_last = false;

  for (std::int64_t epoch = 0; epoch < options.execution.max_epochs; ++epoch) {
    const auto trained_epochs = epoch + 1;
    const bool should_validate =
        epoch >= options.validation.start_from_epoch &&
        ((epoch - options.validation.start_from_epoch) % options.validation.every_n_epochs == 0);
    if (!should_validate && trained_epochs != options.execution.max_epochs) {
      continue;
    }

    last_stage = train_stage(trained_epochs);
    have_last = true;
    std::vector<std::string_view> val_label_views{};
    val_label_views.reserve(val_labels.size());
    for (const auto& label : val_labels) val_label_views.push_back(label);
    const auto report = assess_impl_(last_stage,
                                     val_docs,
                                     pluss::msharp::RangeView::Of<std::string_view>(val_label_views),
                                     static_cast<std::int64_t>(fresh_impl->label_values.size()));
    const double current_metric = choose_metric_(report, options.validation);
    if (!best_snapshot.has_value() ||
        monitor_improved_(current_metric, best_metric, options.validation, options.loop_regularization.min_delta)) {
      best_metric = current_metric;
      stale = 0;
      best_snapshot = last_stage.dump_state_lines();
    } else {
      ++stale;
      if (options.loop_regularization.early_stopping &&
          stale >= options.loop_regularization.patience) {
        break;
      }
    }
  }

  PLUSS_CHECK(have_last,
              "pluss::ai::ice::models::TextJudge: validation loop produced no trained stage.");
  if (options.loop_regularization.restore_best_weights && best_snapshot.has_value()) {
    fresh_impl->stage = TextCatStage(textcat_options_(options_, options, 1));
    std::vector<std::string_view> best_views{};
    best_views.reserve(best_snapshot->size());
    for (const auto& line : *best_snapshot) best_views.push_back(line);
    fresh_impl->stage.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(best_views));
  } else {
    fresh_impl->stage = std::move(last_stage);
  }

  fresh_impl->trained = true;
  impl_ = std::move(fresh_impl);
}

void TextJudge::learn(pluss::msharp::RangeView::Of<std::string_view> texts,
                      pluss::msharp::RangeView::Of<std::string_view> labels,
                      const TextJudgeTrainOptions& options) {
  learn(infer_docs_(options_, texts), labels, options);
}

void TextJudge::learn(const std::vector<std::string>& texts,
                      const std::vector<std::string>& labels,
                      const TextJudgeTrainOptions& options) {
  std::vector<std::string_view> label_views{};
  label_views.reserve(labels.size());
  for (const auto& label : labels) label_views.push_back(label);
  learn(infer_docs_(options_, texts),
        pluss::msharp::RangeView::Of<std::string_view>(label_views),
        options);
}

void TextJudge::learn(const pluss::Note& texts,
                      const pluss::Note& labels,
                      const TextJudgeTrainOptions& options) {
  learn(note_strings_(texts), note_strings_(labels), options);
}

std::vector<TextJudgePrediction> TextJudge::predict(const std::vector<Doc>& docs) const {
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::TextJudge::predict: model has not been trained.");
  return predict_impl_(impl_->stage, docs, impl_->label_values);
}

std::vector<TextJudgePrediction> TextJudge::predict(
    pluss::msharp::RangeView::Of<std::string_view> texts) const {
  return predict(infer_docs_(options_, texts));
}

std::vector<TextJudgePrediction> TextJudge::predict(
    const std::vector<std::string>& texts) const {
  return predict(infer_docs_(options_, texts));
}

std::vector<TextJudgePrediction> TextJudge::predict(const pluss::Note& texts) const {
  return predict(note_strings_(texts));
}

TextJudgeReport TextJudge::assess(const std::vector<Doc>& docs,
                                  pluss::msharp::RangeView::Of<std::string_view> labels) const {
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::TextJudge::assess: model has not been trained.");
  return assess_impl_(impl_->stage, docs, labels, static_cast<std::int64_t>(impl_->label_values.size()));
}

TextJudgeReport TextJudge::assess(pluss::msharp::RangeView::Of<std::string_view> texts,
                                  pluss::msharp::RangeView::Of<std::string_view> labels) const {
  return assess(infer_docs_(options_, texts), labels);
}

TextJudgeReport TextJudge::assess(const std::vector<std::string>& texts,
                                  const std::vector<std::string>& labels) const {
  std::vector<std::string_view> label_views{};
  label_views.reserve(labels.size());
  for (const auto& label : labels) label_views.push_back(label);
  return assess(infer_docs_(options_, texts),
                pluss::msharp::RangeView::Of<std::string_view>(label_views));
}

TextJudgeReport TextJudge::assess(const pluss::Note& texts,
                                  const pluss::Note& labels) const {
  return assess(note_strings_(texts), note_strings_(labels));
}

void TextJudge::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(impl_ != nullptr && impl_->trained,
              "pluss::ai::ice::models::TextJudge::save: model has not been trained.");
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::TextJudge::save: output path cannot be empty.");

  std::ostringstream out{};
  out << kMagic << '\n';
  write_num_line(out, "schema", kSchema);
  write_num_line(out, "width", options_.width);
  write_num_line(out, "dropout", options_.dropout);
  write_bool_line(out, "use_lemmas", options_.use_lemmas);
  write_bool_line(out, "skip_punct", options_.skip_punct);
  write_bool_line(out, "lowercase", options_.lowercase);
  write_num_line(out, "cfg_dtype", static_cast<int>(options_.cfg.dtype));
  write_num_line(out, "cfg_platform", static_cast<int>(options_.cfg.platform));
  write_num_line(out, "seed", options_.seed);

  write_num_line(out, "label_count", static_cast<std::int64_t>(impl_->label_values.size()));
  for (std::size_t i = 0; i < impl_->label_values.size(); ++i) {
    write_kv_line(out, "label." + std::to_string(i), impl_->label_values[i]);
  }

  const auto stage_lines = impl_->stage.dump_state_lines();
  write_num_line(out, "stage_count", static_cast<std::int64_t>(stage_lines.size()));
  for (std::size_t i = 0; i < stage_lines.size(); ++i) {
    write_kv_line(out, "stage." + std::to_string(i), stage_lines[i]);
  }

  std::ofstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TextJudge::save: failed to open '" + std::string(filepath) + "'.");
  file << out.str();
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TextJudge::save: failed while writing '" + std::string(filepath) + "'.");
}

TextJudge TextJudge::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(!filepath.empty(),
              "pluss::ai::ice::models::TextJudge::load: input path cannot be empty.");
  std::ifstream file{std::string(filepath)};
  PLUSS_CHECK(file.good(),
              "pluss::ai::ice::models::TextJudge::load: failed to open '" + std::string(filepath) + "'.");
  std::string magic{};
  PLUSS_CHECK(std::getline(file, magic) && magic == kMagic,
              "pluss::ai::ice::models::TextJudge::load: invalid model artifact header.");
  std::ostringstream body{};
  body << file.rdbuf();
  const auto kv = parse_kv_body(body.str(), kLoadContext);
  const auto schema = require_i64(kv, "schema", kLoadContext);
  PLUSS_CHECK(schema == 2 || schema == kSchema,
              "pluss::ai::ice::models::TextJudge::load: unsupported schema.");

  TextJudgeOptions options{};
  options.width = require_i64(kv, "width", kLoadContext);
  options.dropout = require_f64(kv, "dropout", kLoadContext);
  options.use_lemmas = require_bool(kv, "use_lemmas", kLoadContext);
  options.skip_punct = require_bool(kv, "skip_punct", kLoadContext);
  options.lowercase = require_bool(kv, "lowercase", kLoadContext);
  options.cfg = Config{}
                    .with_dtype(static_cast<ScalarType>(require_i64(kv, "cfg_dtype", kLoadContext)))
                    .with_platform(static_cast<Platform>(require_i64(kv, "cfg_platform", kLoadContext)));
  options.seed = require_u64(kv, "seed", kLoadContext);

  TextJudge model(options);
  auto restored = std::make_shared<Impl>();
  restored->stage = TextCatStage(textcat_options_(options, TextJudgeTrainOptions{}, 1));
  const auto stage_lines = collect_indexed(kv, "stage", require_i64(kv, "stage_count", kLoadContext), kLoadContext);
  std::vector<std::string_view> stage_views{};
  stage_views.reserve(stage_lines.size());
  for (const auto& line : stage_lines) stage_views.push_back(line);
  restored->stage.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(stage_views));
  restored->label_values = collect_indexed(kv, "label", require_i64(kv, "label_count", kLoadContext), kLoadContext);
  for (std::size_t i = 0; i < restored->label_values.size(); ++i) {
    restored->label_to_index.emplace(restored->label_values[i], static_cast<std::int64_t>(i));
  }
  restored->trained = true;
  model.impl_ = std::move(restored);
  return model;
}

const std::vector<std::string>& TextJudge::labels() const noexcept {
  static const std::vector<std::string> empty{};
  return impl_ ? impl_->label_values : empty;
}

bool TextJudge::ready() const noexcept {
  return impl_ != nullptr && impl_->trained;
}

TextJudge& TextJudge::to(Platform platform) {
  options_.cfg = options_.cfg.with_platform(platform);
  return *this;
}

} // namespace pluss::ai::ice::models
