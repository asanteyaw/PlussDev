#include "pluss/ai/nodeb/io.h"
#include "pluss/ai/nodeb/workflow.h"

#include <fstream>
#include <utility>

namespace pluss::ai::nodeb {

namespace {

void validate_training_docs(const std::vector<Doc>& docs,
                            const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  if (docs.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::learn: expected at least one document.");
  }
  if (!options.allow_empty_docs) {
    for (const auto& doc : docs) {
      if (doc.text.empty()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::Workflow::learn: empty document encountered while allow_empty_docs=false.");
      }
    }
  }
}

void validate_labeled_training_docs(const std::vector<Doc>& docs,
                                    pluss::msharp::RangeView::Of<std::string_view> labels,
                                    const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_training_docs(docs, options);
  if (labels.size() != docs.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::learn: label count must match document count.");
  }
}

WorkflowReport make_report(const std::vector<Doc>& docs,
                           const AssessOptions& options) {
  PLUSS_CALL_GUARD();
  WorkflowReport report{};
  report.doc_count = static_cast<std::int64_t>(docs.size());
  for (const auto& doc : docs) {
    report.token_count += static_cast<std::int64_t>(doc.tokens.size());
    report.sentence_count += static_cast<std::int64_t>(doc.sentences.size());
  }
  if (report.doc_count > 0) {
    report.mean_tokens_per_doc =
        static_cast<double>(report.token_count) / static_cast<double>(report.doc_count);
    report.mean_sentences_per_doc =
        static_cast<double>(report.sentence_count) / static_cast<double>(report.doc_count);
  }
  if (options.require_sentences) {
    for (const auto& doc : docs) {
      if (!doc.text.empty() && doc.sentences.empty()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::Workflow::assess: sentence spans required but missing from one or more docs.");
      }
    }
  }
  return report;
}

} // namespace

Workflow& Workflow::append_stage(Stage stage) {
  PLUSS_CALL_GUARD();
  if (!stage) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::append_stage: stage cannot be null.");
  }
  stages.push_back(std::move(stage));
  return *this;
}

Workflow& Workflow::learn(pluss::msharp::RangeView::Of<std::string_view> texts,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  return learn(infer(texts), options);
}

Workflow& Workflow::learn(const std::vector<std::string>& texts,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  std::vector<std::string_view> views;
  views.reserve(texts.size());
  for (const auto& value : texts) {
    views.push_back(value);
  }
  return learn(pluss::msharp::RangeView::Of<std::string_view>(views), options);
}

Workflow& Workflow::learn(const pluss::Note& texts,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  return learn(infer(texts), options);
}

Workflow& Workflow::learn(const std::vector<Doc>& docs,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_training_docs(docs, options);
  for (const auto& stage : stages) {
    if (!stage) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::Workflow::learn: encountered null stage.");
    }
    stage->learn(docs, options);
  }
  return *this;
}

Workflow& Workflow::learn(pluss::msharp::RangeView::Of<std::string_view> texts,
                          pluss::msharp::RangeView::Of<std::string_view> labels,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  return learn(infer(texts), labels, options);
}

Workflow& Workflow::learn(const std::vector<std::string>& texts,
                          const std::vector<std::string>& labels,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  std::vector<std::string_view> text_views;
  text_views.reserve(texts.size());
  for (const auto& value : texts) {
    text_views.push_back(value);
  }
  std::vector<std::string_view> label_views;
  label_views.reserve(labels.size());
  for (const auto& value : labels) {
    label_views.push_back(value);
  }
  return learn(pluss::msharp::RangeView::Of<std::string_view>(text_views),
               pluss::msharp::RangeView::Of<std::string_view>(label_views),
               options);
}

Workflow& Workflow::learn(const pluss::Note& texts,
                          const pluss::Note& labels,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  std::vector<std::string> label_store;
  label_store.reserve(static_cast<std::size_t>(labels.numel()));
  for (std::int64_t i = 0; i < labels.numel(); ++i) {
    label_store.emplace_back(labels.at(i));
  }
  std::vector<std::string_view> label_views;
  label_views.reserve(label_store.size());
  for (const auto& value : label_store) {
    label_views.push_back(value);
  }
  return learn(infer(texts),
               pluss::msharp::RangeView::Of<std::string_view>(label_views),
               options);
}

Workflow& Workflow::learn(const std::vector<Doc>& docs,
                          pluss::msharp::RangeView::Of<std::string_view> labels,
                          const TrainOptions& options) {
  PLUSS_CALL_GUARD();
  validate_labeled_training_docs(docs, labels, options);
  for (const auto& stage : stages) {
    if (!stage) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::Workflow::learn: encountered null stage.");
    }
    stage->learn(docs, labels, options);
  }
  return *this;
}

Doc Workflow::infer(std::string_view text) const {
  PLUSS_CALL_GUARD();
  Doc doc{};
  doc.text = std::string(text);
  doc.lang = lang;
  for (const auto& stage : stages) {
    if (!stage) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::Workflow::infer: encountered null stage.");
    }
    stage->apply(doc);
  }
  return doc;
}

std::vector<Doc> Workflow::infer(pluss::msharp::RangeView::Of<std::string_view> texts) const {
  PLUSS_CALL_GUARD();
  std::vector<Doc> out;
  out.reserve(texts.size());
  for (std::size_t i = 0; i < texts.size(); ++i) {
    out.push_back(infer(texts[i]));
  }
  return out;
}

std::vector<Doc> Workflow::infer(const std::vector<std::string>& texts) const {
  PLUSS_CALL_GUARD();
  std::vector<std::string_view> views;
  views.reserve(texts.size());
  for (const auto& value : texts) {
    views.push_back(value);
  }
  return infer(pluss::msharp::RangeView::Of<std::string_view>(views));
}

std::vector<Doc> Workflow::infer(const pluss::Note& texts) const {
  PLUSS_CALL_GUARD();
  std::vector<Doc> out;
  out.reserve(static_cast<std::size_t>(texts.numel()));
  for (std::int64_t i = 0; i < texts.numel(); ++i) {
    out.push_back(infer(texts.at(i)));
  }
  return out;
}

WorkflowReport Workflow::assess(pluss::msharp::RangeView::Of<std::string_view> texts,
                                const AssessOptions& options) const {
  PLUSS_CALL_GUARD();
  return make_report(infer(texts), options);
}

WorkflowReport Workflow::assess(const std::vector<std::string>& texts,
                                const AssessOptions& options) const {
  PLUSS_CALL_GUARD();
  return make_report(infer(texts), options);
}

WorkflowReport Workflow::assess(const pluss::Note& texts,
                                const AssessOptions& options) const {
  PLUSS_CALL_GUARD();
  return make_report(infer(texts), options);
}

WorkflowReport Workflow::assess(const std::vector<Doc>& docs,
                                const AssessOptions& options) const {
  PLUSS_CALL_GUARD();
  return make_report(docs, options);
}

void Workflow::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::save: output path cannot be empty.");
  }
  std::ofstream out{std::string(filepath)};
  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::save: failed to open '" + std::string(filepath) + "'.");
  }
  io::write_workflow(out, *this);
  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::save: failed while writing '" + std::string(filepath) + "'.");
  }
}

Workflow Workflow::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::load: input path cannot be empty.");
  }
  std::ifstream in{std::string(filepath)};
  if (!in.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::Workflow::load: failed to open '" + std::string(filepath) + "'.");
  }
  return io::read_workflow(in);
}

} // namespace pluss::ai::nodeb
