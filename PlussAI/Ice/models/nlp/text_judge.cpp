#include "pluss/ai/ice/models/nlp/text_judge.h"

#include "pluss/ai/nodeb/tasks.h"
#include "pluss/ai/nodeb/tokenize.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
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
using StringMap = std::unordered_map<std::string, std::string>;

constexpr std::string_view kMagic = "pluss_ice_text_judge";
constexpr std::int64_t kSchema = 2;

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
                                              const TextJudgeTrainOptions& train_options) {
  TextCatOptions options{};
  options.lowercase = model_options.lowercase;
  options.use_lemmas = model_options.use_lemmas;
  options.backend = TaskBackend::Sequence;
  options.sequence_width = model_options.width;
  options.sequence_seed = model_options.seed;
  options.max_iterations = std::max<std::int64_t>(32, train_options.execution.max_epochs);
  options.tolerance = std::max(1e-8, train_options.loop_regularization.min_delta);
  return options;
}

[[nodiscard]] TrainOptions nodeb_train_options_(const TextJudgeTrainOptions& options) {
  TrainOptions out{};
  out.random.seed = options.seed;
  out.random.deterministic = true;
  return out;
}

void write_kv_(std::ostringstream& out, std::string_view key, std::string_view value) {
  out << key << '=';
  out << std::quoted(std::string(value)) << '\n';
}

template <class T>
void write_num_(std::ostringstream& out, std::string_view key, T value) {
  out << key << '=' << value << '\n';
}

void write_bool_(std::ostringstream& out, std::string_view key, bool value) {
  out << key << '=' << (value ? 1 : 0) << '\n';
}

[[nodiscard]] StringMap parse_body_(std::string_view body) {
  StringMap out{};
  std::istringstream stream{std::string(body)};
  std::string line{};
  while (std::getline(stream, line)) {
    if (line.empty()) continue;
    const auto eq = line.find('=');
    PLUSS_CHECK(eq != std::string::npos,
                "pluss::ai::ice::models::TextJudge::load: malformed metadata line '" + line + "'.");
    std::string key = line.substr(0, eq);
    std::string value_text = line.substr(eq + 1);
    std::string value{};
    std::istringstream value_stream(value_text);
    if (!value_text.empty() && value_text.front() == '"') {
      value_stream >> std::quoted(value);
    } else {
      value = value_text;
    }
    out.emplace(std::move(key), std::move(value));
  }
  return out;
}

[[nodiscard]] std::string require_(const StringMap& kv, std::string_view key) {
  const auto it = kv.find(std::string(key));
  PLUSS_CHECK(it != kv.end(),
              "pluss::ai::ice::models::TextJudge::load: missing field '" + std::string(key) + "'.");
  return it->second;
}

[[nodiscard]] std::int64_t require_i64_(const StringMap& kv, std::string_view key) {
  return std::stoll(require_(kv, key));
}

[[nodiscard]] std::uint64_t require_u64_(const StringMap& kv, std::string_view key) {
  return static_cast<std::uint64_t>(std::stoull(require_(kv, key)));
}

[[nodiscard]] double require_f64_(const StringMap& kv, std::string_view key) {
  return std::stod(require_(kv, key));
}

[[nodiscard]] bool require_bool_(const StringMap& kv, std::string_view key) {
  const auto value = require_(kv, key);
  return value == "1" || value == "true";
}

[[nodiscard]] std::vector<std::string> collect_indexed_(const StringMap& kv,
                                                        std::string_view prefix,
                                                        std::int64_t count) {
  std::vector<std::string> out(static_cast<std::size_t>(std::max<std::int64_t>(0, count)));
  for (std::int64_t i = 0; i < count; ++i) {
    out[static_cast<std::size_t>(i)] = require_(kv, std::string(prefix) + "." + std::to_string(i));
  }
  return out;
}

void validate_model_options_(const TextJudgeOptions& options) {
  PLUSS_CHECK(options.width > 0,
              "pluss::ai::ice::models::TextJudge: width must be positive.");
  PLUSS_CHECK(options.dropout >= 0.0 && options.dropout <= 1.0,
              "pluss::ai::ice::models::TextJudge: dropout must be in [0, 1].");
}

void validate_train_options_(const TextJudgeTrainOptions& options) {
  PLUSS_CHECK(options.execution.max_epochs > 0,
              "pluss::ai::ice::models::TextJudge: execution.max_epochs must be positive.");
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
  fresh_impl->stage = TextCatStage(textcat_options_(options_, options));
  fresh_impl->stage.learn(docs, labels, nodeb_train_options_(options));

  for (std::size_t i = 0; i < labels.size(); ++i) {
    const std::string label(labels[i]);
    if (!fresh_impl->label_to_index.contains(label)) {
      fresh_impl->label_to_index.emplace(label, static_cast<std::int64_t>(fresh_impl->label_values.size()));
      fresh_impl->label_values.push_back(label);
    }
  }
  PLUSS_CHECK(fresh_impl->label_values.size() >= 2,
              "pluss::ai::ice::models::TextJudge: expected at least two distinct labels.");
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
  write_num_(out, "schema", kSchema);
  write_num_(out, "width", options_.width);
  write_num_(out, "num_layers", options_.num_layers);
  write_num_(out, "num_heads", options_.num_heads);
  write_num_(out, "feedforward_width", options_.feedforward_width);
  write_num_(out, "dropout", options_.dropout);
  write_num_(out, "max_positions", options_.max_positions);
  write_bool_(out, "use_lemmas", options_.use_lemmas);
  write_bool_(out, "skip_punct", options_.skip_punct);
  write_bool_(out, "lowercase", options_.lowercase);
  write_num_(out, "cfg_dtype", static_cast<int>(options_.cfg.dtype));
  write_num_(out, "cfg_platform", static_cast<int>(options_.cfg.platform));
  write_num_(out, "seed", options_.seed);

  write_num_(out, "label_count", static_cast<std::int64_t>(impl_->label_values.size()));
  for (std::size_t i = 0; i < impl_->label_values.size(); ++i) {
    write_kv_(out, "label." + std::to_string(i), impl_->label_values[i]);
  }

  const auto stage_lines = impl_->stage.dump_state_lines();
  write_num_(out, "stage_count", static_cast<std::int64_t>(stage_lines.size()));
  for (std::size_t i = 0; i < stage_lines.size(); ++i) {
    write_kv_(out, "stage." + std::to_string(i), stage_lines[i]);
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
  const auto kv = parse_body_(body.str());
  PLUSS_CHECK(require_i64_(kv, "schema") == kSchema,
              "pluss::ai::ice::models::TextJudge::load: unsupported schema.");

  TextJudgeOptions options{};
  options.width = require_i64_(kv, "width");
  options.num_layers = require_i64_(kv, "num_layers");
  options.num_heads = require_i64_(kv, "num_heads");
  options.feedforward_width = require_i64_(kv, "feedforward_width");
  options.dropout = require_f64_(kv, "dropout");
  options.max_positions = require_i64_(kv, "max_positions");
  options.use_lemmas = require_bool_(kv, "use_lemmas");
  options.skip_punct = require_bool_(kv, "skip_punct");
  options.lowercase = require_bool_(kv, "lowercase");
  options.cfg = Config{}
                    .with_dtype(static_cast<ScalarType>(require_i64_(kv, "cfg_dtype")))
                    .with_platform(static_cast<Platform>(require_i64_(kv, "cfg_platform")));
  options.seed = require_u64_(kv, "seed");

  TextJudge model(options);
  auto restored = std::make_shared<Impl>();
  restored->stage = TextCatStage(textcat_options_(options, TextJudgeTrainOptions{}));
  const auto stage_lines = collect_indexed_(kv, "stage", require_i64_(kv, "stage_count"));
  std::vector<std::string_view> stage_views{};
  stage_views.reserve(stage_lines.size());
  for (const auto& line : stage_lines) stage_views.push_back(line);
  restored->stage.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(stage_views));
  restored->label_values = collect_indexed_(kv, "label", require_i64_(kv, "label_count"));
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
