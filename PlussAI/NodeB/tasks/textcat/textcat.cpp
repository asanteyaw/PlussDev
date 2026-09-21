#include "pluss/ai/nodeb/tasks.h"
#include "../deep/sequence_heads.h"

#include "pluss/ai/ice/optim.h"
#include "pluss/ai/nodej/text.h"
#include "pluss/ai/nodej/sparse_runtime.h"
#include "pluss/nn.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::nodeb {

namespace {

std::vector<std::string> build_feature_docs(const std::vector<Doc>& docs,
                                            bool use_lemmas) {
  std::vector<std::string> out;
  out.reserve(docs.size());
  for (const auto& doc : docs) {
    std::string text;
    for (const auto& token : doc.tokens) {
      if (token.is_punct) continue;
      const std::string_view piece =
          (use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                               : std::string_view(token.orth);
      if (piece.empty()) continue;
      if (!text.empty()) text.push_back(' ');
      text.append(piece.data(), piece.size());
    }
    if (text.empty()) text = doc.text;
    out.push_back(std::move(text));
  }
  return out;
}

// Term extraction for the Sequence backend's own small trainable
// vocabulary. Deliberately independent of `build_feature_docs` above
// (which joins terms into a single tf-idf-ready string for the Classic
// backend's `LexiconMapper`) since the Sequence backend needs a per-token
// id sequence instead.
[[nodiscard]] std::vector<std::string> select_terms(const Doc& doc,
                                                    bool use_lemmas,
                                                    bool lowercase,
                                                    bool skip_punct) {
  std::vector<std::string> terms;
  terms.reserve(doc.tokens.size());
  for (const auto& token : doc.tokens) {
    if (skip_punct && token.is_punct) continue;
    const std::string_view piece =
        (use_lemmas && !token.lemma.empty()) ? std::string_view(token.lemma)
                                             : std::string_view(token.orth);
    if (piece.empty()) continue;
    std::string term(piece);
    if (lowercase) {
      std::ranges::transform(term, term.begin(),
                             [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }
    terms.push_back(std::move(term));
  }
  return terms;
}

[[nodiscard]] Config sequence_float_config() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

[[nodiscard]] Config sequence_index_config() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::vector<std::vector<double>> to_host_matrix(const Series& values,
                                                               std::int64_t rows,
                                                               std::int64_t cols) {
  const auto flat = values.to(pluss::pCPU, ScalarType::Float64).reshape({rows * cols}).to_vec<double>();
  std::vector<std::vector<double>> out(static_cast<std::size_t>(rows));
  for (std::int64_t r = 0; r < rows; ++r) {
    const auto offset = static_cast<std::size_t>(r * cols);
    out[static_cast<std::size_t>(r)].assign(
        flat.begin() + static_cast<std::ptrdiff_t>(offset),
        flat.begin() + static_cast<std::ptrdiff_t>(offset + static_cast<std::size_t>(cols)));
  }
  return out;
}

// --- Shared building blocks for TaskBackend::Sequence (both the
// single-label and multi-label training paths jointly train their own
// embedding table end to end with a linear head; only the loss/targets
// differ). ---

struct SequenceVocab {
  std::vector<std::string> vocabulary{};
  std::unordered_map<std::string, std::int64_t> token_to_id{};
};

[[nodiscard]] SequenceVocab build_sequence_vocab(const std::vector<std::vector<std::string>>& doc_terms) {
  SequenceVocab vocab{};
  for (const auto& terms : doc_terms) {
    for (const auto& term : terms) {
      if (!vocab.token_to_id.contains(term)) {
        vocab.token_to_id.emplace(term, static_cast<std::int64_t>(vocab.vocabulary.size()));
        vocab.vocabulary.push_back(term);
      }
    }
  }
  return vocab;
}

[[nodiscard]] std::vector<Series> build_doc_id_series(const std::vector<std::vector<std::string>>& doc_terms,
                                                       const std::unordered_map<std::string, std::int64_t>& token_to_id,
                                                       const Config& idx_cfg) {
  std::vector<Series> doc_ids{};
  doc_ids.reserve(doc_terms.size());
  for (const auto& terms : doc_terms) {
    std::vector<std::int64_t> ids{};
    ids.reserve(terms.size());
    for (const auto& term : terms) {
      ids.push_back(token_to_id.at(term));
    }
    if (ids.empty()) {
      ids.push_back(0); // a doc contributing no terms (e.g. all punctuation) falls back to vocab id 0.
    }
    doc_ids.push_back(
        pluss::make_series(ids, std::vector<std::int64_t>{static_cast<std::int64_t>(ids.size())}, idx_cfg));
  }
  return doc_ids;
}

[[nodiscard]] Series pool_docs(const Series& embedding_weights, const std::vector<Series>& doc_ids) {
  std::vector<Series> pooled_rows{};
  pooled_rows.reserve(doc_ids.size());
  for (const auto& ids : doc_ids) {
    pooled_rows.push_back(embedding_weights.index_select(0, ids).mean(0)); // [width]
  }
  return pluss::series::pile(pooled_rows, 0); // [docs, width]
}

[[nodiscard]] std::vector<std::string> split_multi_label(std::string_view value) {
  std::vector<std::string> out{};
  std::size_t start = 0;
  while (start <= value.size()) {
    const auto sep = value.find('|', start);
    const auto end = (sep == std::string_view::npos) ? value.size() : sep;
    const auto piece = value.substr(start, end - start);
    if (!piece.empty()) out.emplace_back(piece);
    if (sep == std::string_view::npos) break;
    start = sep + 1;
  }
  return out;
}

double sigmoid(double x) {
  if (x >= 0.0) {
    const double e = std::exp(-x);
    return 1.0 / (1.0 + e);
  }
  const double e = std::exp(x);
  return e / (1.0 + e);
}

// hex_encode_string/hex_decode_string/join_doubles_line/parse_doubles_line
// live in sequence_heads.h (already included above) — this used to keep its
// own byte-for-byte duplicate of hex_encode/hex_decode and the double
// overload of what's now join_numbers, instead of using them. join_numbers
// stays here as a template only because LexiconMapperState::document_frequency
// is a vector<int64_t>, the one caller below that isn't vector<double>.
template <class T>
[[nodiscard]] std::string join_numbers(const std::vector<T>& values) {
  std::ostringstream oss;
  oss.precision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::vector<std::int64_t> parse_i64s(std::string_view value) {
  std::vector<std::int64_t> out;
  std::istringstream iss{std::string(value)};
  std::int64_t parsed = 0;
  while (iss >> parsed) out.push_back(parsed);
  return out;
}

[[nodiscard]] std::vector<pluss::Series> parse_series_state_lines(std::string_view prefix,
                                                                  pluss::msharp::RangeView::Of<std::string_view> lines) {
  std::int64_t count = -1;
  std::vector<std::vector<double>> payloads;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    const std::string count_key = std::string(prefix) + "_count";
    if (key == count_key) {
      count = std::stoll(std::string(value));
      payloads.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, count)), {});
      continue;
    }
    const std::string item_prefix = std::string(prefix) + "_";
    if (key.rfind(item_prefix, 0) != 0) continue;
    const std::string_view suffix(key.data() + item_prefix.size(), key.size() - item_prefix.size());
    if (suffix.empty() ||
        !std::all_of(suffix.begin(), suffix.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      continue;
    }
    const auto idx = static_cast<std::size_t>(std::stoll(key.substr(item_prefix.size())));
    if (idx >= payloads.size()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::load_state_lines: state tensor index out of range.");
    }
    payloads[idx] = detail::parse_doubles_line(value);
  }
  if (count < 0) return {};
  std::vector<pluss::Series> out;
  out.reserve(payloads.size());
  for (auto& values : payloads) {
    const auto count_i64 = static_cast<std::int64_t>(values.size());
    out.push_back(pluss::make_series(std::move(values), {count_i64}));
  }
  return out;
}

} // namespace

struct TextCatStage::Impl {
  ::pluss::ai::nodej::LexiconMapper mapper{};
  ::pluss::ai::nodej::SparseLinearBinaryClassifier binary{};
  ::pluss::ai::nodej::SparseLinearMultiClassifier multi{};
  std::vector<std::string> label_values{};
  bool trained = false;
  bool multiclass = false;

  // TaskBackend::Sequence state: a small embedding table and a linear
  // softmax head, jointly trained end to end via real backpropagation
  // (see TextCatStage::learn). Stored as plain host vectors post-training
  // so inference in `apply()` never needs to touch the autodiff graph.
  std::vector<std::string> sequence_vocabulary{};
  std::unordered_map<std::string, std::int64_t> sequence_token_to_id{};
  std::vector<std::vector<double>> sequence_embedding{};   // [vocab, width]
  std::vector<std::vector<double>> sequence_cls_weight{};  // [width, classes]
  std::vector<double> sequence_cls_bias{};                 // [classes]
};

TextCatStage::TextCatStage(TextCatOptions options)
    : impl_(std::make_shared<Impl>()),
      options_(options) {}

TextCatStage::~TextCatStage() = default;

std::string_view TextCatStage::name() const noexcept {
  return "textcat";
}

void TextCatStage::learn(const std::vector<Doc>& docs,
                         pluss::msharp::RangeView::Of<std::string_view> labels,
                         const TrainOptions&) {
  PLUSS_CALL_GUARD();
  if (labels.size() != docs.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: label count must match document count.");
  }
  if (docs.empty()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: expected at least one document.");
  }
  if (options_.multi_label && options_.backend != TaskBackend::Sequence) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: multi_label is only supported with TaskBackend::Sequence.");
  }
  if (options_.multi_label) {
    if (options_.sequence_width <= 0) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: sequence_width must be positive for the Sequence backend.");
    }
    impl_->mapper = ::pluss::ai::nodej::LexiconMapper(::pluss::ai::nodej::LexiconMapOptions{});
    impl_->binary = decltype(impl_->binary){};
    impl_->multi = decltype(impl_->multi){};
    impl_->multiclass = false; // multi-label predictions are independent per-label, not one-of-N.

    std::vector<std::string> label_values{};
    std::unordered_map<std::string, std::int64_t> label_index{};
    std::vector<std::vector<std::int64_t>> doc_label_ids{};
    doc_label_ids.reserve(labels.size());
    for (std::size_t i = 0; i < labels.size(); ++i) {
      std::vector<std::int64_t> ids{};
      for (const auto& label : split_multi_label(labels[i])) {
        const auto it = label_index.find(label);
        if (it == label_index.end()) {
          const auto index = static_cast<std::int64_t>(label_values.size());
          label_index.emplace(label, index);
          label_values.push_back(label);
          ids.push_back(index);
        } else {
          ids.push_back(it->second);
        }
      }
      doc_label_ids.push_back(std::move(ids));
    }
    if (label_values.size() < 2) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: expected at least two distinct labels.");
    }
    impl_->label_values = label_values;

    std::vector<std::vector<std::string>> doc_terms{};
    doc_terms.reserve(docs.size());
    for (const auto& doc : docs) {
      doc_terms.push_back(select_terms(doc, options_.use_lemmas, options_.lowercase, options_.skip_punct));
    }
    auto vocab = build_sequence_vocab(doc_terms);
    if (vocab.vocabulary.empty()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: no vocabulary terms found for the Sequence backend.");
    }

    const auto vocab_size = static_cast<std::int64_t>(vocab.vocabulary.size());
    const auto num_classes = static_cast<std::int64_t>(label_values.size());
    const auto width = options_.sequence_width;
    const auto cfg = sequence_float_config();
    const auto idx_cfg = sequence_index_config();
    const double scale = 0.5 / static_cast<double>(width);

    Series embedding_weights =
        pluss::series::rand({vocab_size, width}, cfg, options_.sequence_seed).sub(0.5).mul(2.0 * scale);
    Series cls_weight =
        pluss::series::rand({width, num_classes}, cfg, options_.sequence_seed + 7).sub(0.5).mul(2.0 * scale);
    Series cls_bias = pluss::series::zeros({num_classes}, cfg);
    embedding_weights.tracks_slope_(true);
    cls_weight.tracks_slope_(true);
    cls_bias.tracks_slope_(true);

    const auto doc_ids = build_doc_id_series(doc_terms, vocab.token_to_id, idx_cfg);

    // `make_series(vector<T>, ...)` infers the tensor dtype from `T` rather
    // than from `cfg`, unlike `pluss::rand`/`pluss::ones`/`pluss::zeros` —
    // use `float` so the target lands in Float32, matching the logits.
    std::vector<float> multi_hot(docs.size() * static_cast<std::size_t>(num_classes), 0.0F);
    for (std::size_t i = 0; i < doc_label_ids.size(); ++i) {
      for (const auto label_id : doc_label_ids[i]) {
        multi_hot[i * static_cast<std::size_t>(num_classes) + static_cast<std::size_t>(label_id)] = 1.0F;
      }
    }
    const Series y_series = pluss::make_series(
        multi_hot, std::vector<std::int64_t>{static_cast<std::int64_t>(docs.size()), num_classes}, cfg);

    // Vocab-sized architecture discovered from the data + a structured
    // (non-Series) pooled-embedding input means this loop doesn't fit
    // ice::Model's generic forward/loss shape (same reasoning as
    // embed::Embedder) — but ice::make_optimizer still applies, and gives
    // this loop gradient clipping and non-finite diagnostics it previously
    // had none of at all.
    std::vector<Series*> parameters{&embedding_weights, &cls_weight, &cls_bias};
    pluss::ai::ice::TrainOptions optimizer_options{};
    optimizer_options.optimizer = pluss::ai::ice::OptimizerKind::Adam;
    optimizer_options.learning_rate = options_.sequence_learning_rate;
    auto optimizer = pluss::ai::ice::make_optimizer(parameters, optimizer_options);

    double previous_loss = std::numeric_limits<double>::infinity();
    try {
      for (std::int64_t iter = 0; iter < options_.max_iterations; ++iter) {
        const Series pooled = pool_docs(embedding_weights, doc_ids);   // [docs, width]
        const Series logits = pooled.matmul(cls_weight).add(cls_bias); // [docs, classes]
        const Series loss = pluss::nn::losses::BCEWithLogits(logits, y_series);

        optimizer->clear();
        loss.reverse();
        optimizer->step();

        const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
        if (std::abs(previous_loss - loss_value) < options_.tolerance) {
          previous_loss = loss_value;
          break;
        }
        previous_loss = loss_value;
      }
    } catch (const std::exception& e) {
      ::pluss::ai::nodeb::detail::fail(
          std::string("nodeb::TextCatStage::learn: training became numerically unstable: ") + e.what());
    }

    impl_->sequence_vocabulary = std::move(vocab.vocabulary);
    impl_->sequence_token_to_id = std::move(vocab.token_to_id);
    impl_->sequence_embedding = to_host_matrix(embedding_weights, vocab_size, width);
    impl_->sequence_cls_weight = to_host_matrix(cls_weight, width, num_classes);
    impl_->sequence_cls_bias = cls_bias.to(pluss::pCPU, ScalarType::Float64).reshape({num_classes}).to_vec<double>();
    impl_->trained = true;
    return;
  }

  auto feature_docs = build_feature_docs(docs, options_.use_lemmas);
  ::pluss::ai::nodej::LexiconMapper mapper(::pluss::ai::nodej::LexiconMapOptions{
      .lowercase = options_.lowercase,
      .max_features = options_.max_features,
      .min_document_frequency = options_.min_document_frequency,
      .binary = false,
  });
  auto fit_status = mapper.learn(::pluss::msharp::RangeView::Of<std::string>(feature_docs));
  if (!fit_status.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: " + fit_status.message);
  }

  auto x_or = mapper.map_tfidf_sparse(::pluss::msharp::RangeView::Of<std::string>(feature_docs));
  if (!x_or.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: " + x_or.status().message);
  }

  std::vector<std::string> label_values;
  label_values.reserve(labels.size());
  std::unordered_map<std::string, std::int64_t> label_index;
  std::vector<double> y_values;
  std::vector<std::int64_t> y_ids;
  y_values.reserve(labels.size());
  y_ids.reserve(labels.size());
  for (std::size_t i = 0; i < labels.size(); ++i) {
    const std::string label(labels[i]);
    const auto it = label_index.find(label);
    if (it == label_index.end()) {
      const auto index = static_cast<std::int64_t>(label_values.size());
      label_index.emplace(label, index);
      label_values.push_back(label);
      y_values.push_back(static_cast<double>(index));
      y_ids.push_back(index);
    } else {
      y_values.push_back(static_cast<double>(it->second));
      y_ids.push_back(it->second);
    }
  }
  if (label_values.size() < 2) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::learn: expected at least two distinct labels.");
  }

  if (options_.backend == TaskBackend::Sequence) {
    if (options_.sequence_width <= 0) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: sequence_width must be positive for the Sequence backend.");
    }
    impl_->mapper = ::pluss::ai::nodej::LexiconMapper(::pluss::ai::nodej::LexiconMapOptions{});
    impl_->binary = decltype(impl_->binary){};
    impl_->multi = decltype(impl_->multi){};
    impl_->label_values = std::move(label_values);
    impl_->multiclass = impl_->label_values.size() > 2;

    // Build the Sequence backend's own small vocabulary. This is
    // intentionally independent of `sequence_head`/`sequence_pooling`
    // (the shared tagger/ner/textcat "detail::SequenceTaskState" machinery
    // used by TaskBackend::Sequence there pools a randomly-initialized,
    // never-trained projection): textcat instead jointly trains its own
    // embedding table end to end with a linear softmax head via real
    // backpropagation, always with mean pooling.
    std::vector<std::vector<std::string>> doc_terms;
    doc_terms.reserve(docs.size());
    for (const auto& doc : docs) {
      doc_terms.push_back(select_terms(doc, options_.use_lemmas, options_.lowercase, options_.skip_punct));
    }
    auto vocab = build_sequence_vocab(doc_terms);
    if (vocab.vocabulary.empty()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: no vocabulary terms found for the Sequence backend.");
    }

    const auto vocab_size = static_cast<std::int64_t>(vocab.vocabulary.size());
    const auto num_classes = static_cast<std::int64_t>(impl_->label_values.size());
    const auto width = options_.sequence_width;
    const auto cfg = sequence_float_config();
    const auto idx_cfg = sequence_index_config();
    const double scale = 0.5 / static_cast<double>(width);

    Series embedding_weights =
        pluss::series::rand({vocab_size, width}, cfg, options_.sequence_seed).sub(0.5).mul(2.0 * scale);
    Series cls_weight =
        pluss::series::rand({width, num_classes}, cfg, options_.sequence_seed + 7).sub(0.5).mul(2.0 * scale);
    Series cls_bias = pluss::series::zeros({num_classes}, cfg);
    embedding_weights.tracks_slope_(true);
    cls_weight.tracks_slope_(true);
    cls_bias.tracks_slope_(true);

    const auto doc_ids = build_doc_id_series(doc_terms, vocab.token_to_id, idx_cfg);
    const Series y_series =
        pluss::make_series(y_ids, std::vector<std::int64_t>{static_cast<std::int64_t>(y_ids.size())}, idx_cfg);

    // Same reasoning as the multi-label branch above: doesn't fit
    // ice::Model's shape, but ice::make_optimizer adds the same missing
    // safety net.
    std::vector<Series*> parameters{&embedding_weights, &cls_weight, &cls_bias};
    pluss::ai::ice::TrainOptions optimizer_options{};
    optimizer_options.optimizer = pluss::ai::ice::OptimizerKind::Adam;
    optimizer_options.learning_rate = 0.05;
    auto optimizer = pluss::ai::ice::make_optimizer(parameters, optimizer_options);

    double previous_loss = std::numeric_limits<double>::infinity();
    try {
      for (std::int64_t iter = 0; iter < options_.max_iterations; ++iter) {
        Series pooled = pool_docs(embedding_weights, doc_ids);            // [docs, width]
        pooled = pluss::ai::dropout(
            pooled,
            options_.sequence_dropout,
            /*training=*/options_.sequence_dropout > 0.0,
            options_.sequence_seed + static_cast<std::uint64_t>(iter));
        const Series logits = pooled.matmul(cls_weight).add(cls_bias);    // [docs, classes]
        const Series loss = pluss::nn::losses::CrossEntropy(logits, y_series);

        optimizer->clear();
        loss.reverse();
        optimizer->step();

        const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
        if (std::abs(previous_loss - loss_value) < options_.tolerance) {
          previous_loss = loss_value;
          break;
        }
        previous_loss = loss_value;
      }
    } catch (const std::exception& e) {
      ::pluss::ai::nodeb::detail::fail(
          std::string("nodeb::TextCatStage::learn: training became numerically unstable: ") + e.what());
    }

    impl_->sequence_vocabulary = std::move(vocab.vocabulary);
    impl_->sequence_token_to_id = std::move(vocab.token_to_id);
    impl_->sequence_embedding = to_host_matrix(embedding_weights, vocab_size, width);
    impl_->sequence_cls_weight = to_host_matrix(cls_weight, width, num_classes);
    impl_->sequence_cls_bias = cls_bias.to(pluss::pCPU, ScalarType::Float64).reshape({num_classes}).to_vec<double>();
    impl_->trained = true;
    return;
  }

  const auto y = ::pluss::make_series(std::move(y_values));
  const auto linear_options = ::pluss::ai::nodej::LinearFamilyOptions{};
  const auto train_options = ::pluss::ai::nodej::TrainOptions{
      .max_iterations = options_.max_iterations,
      .tolerance = options_.tolerance,
  };

  impl_->mapper = std::move(mapper);
  impl_->label_values = std::move(label_values);
  impl_->multiclass = impl_->label_values.size() > 2;
  if (impl_->multiclass) {
    impl_->binary = {};
    impl_->multi = {};
    const auto status = impl_->multi.learn(x_or.value(), y, train_options, linear_options);
    if (!status.ok()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: " + status.message);
    }
  } else {
    impl_->binary = {};
    impl_->multi = {};
    const auto status = impl_->binary.learn(x_or.value(), y, train_options, linear_options);
    if (!status.ok()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::learn: " + status.message);
    }
  }
  impl_->trained = true;
}

void TextCatStage::apply(Doc& doc) const {
  PLUSS_CALL_GUARD();
  doc.doc_label.clear();
  doc.doc_score = std::numeric_limits<double>::quiet_NaN();
  doc.doc_labels.clear();
  if (!impl_ || !impl_->trained) return;

  if (options_.backend == TaskBackend::Sequence) {
    if (impl_->sequence_vocabulary.empty() || impl_->sequence_cls_bias.empty()) return;
    const auto terms = select_terms(doc, options_.use_lemmas, options_.lowercase, options_.skip_punct);
    const auto width = static_cast<std::size_t>(options_.sequence_width);
    std::vector<double> pooled(width, 0.0);
    std::int64_t count = 0;
    for (const auto& term : terms) {
      const auto it = impl_->sequence_token_to_id.find(term);
      if (it == impl_->sequence_token_to_id.end()) continue;
      const auto& row = impl_->sequence_embedding[static_cast<std::size_t>(it->second)];
      for (std::size_t i = 0; i < width; ++i) pooled[i] += row[i];
      ++count;
    }
    if (count == 0) return;
    for (auto& value : pooled) value /= static_cast<double>(count);

    std::vector<double> logits(impl_->sequence_cls_bias.size(), 0.0);
    for (std::size_t c = 0; c < logits.size(); ++c) {
      double sum = impl_->sequence_cls_bias[c];
      for (std::size_t i = 0; i < width; ++i) {
        sum += pooled[i] * impl_->sequence_cls_weight[i][c];
      }
      logits[c] = sum;
    }
    if (options_.multi_label) {
      doc.doc_labels.reserve(logits.size());
      for (std::size_t c = 0; c < logits.size() && c < impl_->label_values.size(); ++c) {
        const double probability = sigmoid(logits[c]);
        if (probability >= options_.multi_label_threshold) {
          doc.doc_labels.emplace_back(impl_->label_values[c], probability);
        }
      }
      std::ranges::sort(doc.doc_labels, [](const auto& a, const auto& b) { return a.second > b.second; });
      return;
    }

    const auto best_it = std::max_element(logits.begin(), logits.end());
    const auto best_index = static_cast<std::size_t>(std::distance(logits.begin(), best_it));
    double denom = 0.0;
    for (const double value : logits) denom += std::exp(value - *best_it);
    if (best_index < impl_->label_values.size()) {
      doc.doc_label = impl_->label_values[best_index];
      doc.doc_score = 1.0 / denom; // softmax probability of the argmax class.
    }
    return;
  }

  std::vector<Doc> docs{doc};
  auto feature_docs = build_feature_docs(docs, options_.use_lemmas);
  auto x_or = impl_->mapper.map_tfidf_sparse(::pluss::msharp::RangeView::Of<std::string>(feature_docs));
  if (!x_or.ok()) return;

  if (!impl_->multiclass) {
    auto pred_or = impl_->binary.infer(x_or.value());
    auto score_or = impl_->binary.infer_scores(x_or.value());
    if (!pred_or.ok() || !score_or.ok()) return;
    const auto pred = static_cast<std::int64_t>(pred_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).item<double>());
    const double raw_score = score_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
    if (pred >= 0 && pred < static_cast<std::int64_t>(impl_->label_values.size())) {
      doc.doc_label = impl_->label_values[static_cast<std::size_t>(pred)];
      doc.doc_score = sigmoid(raw_score);
    }
    return;
  }

  auto pred_or = impl_->multi.infer(x_or.value());
  auto score_or = impl_->multi.infer_scores(x_or.value());
  if (!pred_or.ok() || !score_or.ok()) return;
  const auto pred = static_cast<std::int64_t>(pred_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).item<double>());
  const auto raw_scores =
      score_or.value().to(pluss::pCPU, pluss::ScalarType::Float64).reshape({score_or.value().numel()}).to_vec<double>();
  if (pred >= 0 && pred < static_cast<std::int64_t>(impl_->label_values.size())) {
    doc.doc_label = impl_->label_values[static_cast<std::size_t>(pred)];
    if (pred < static_cast<std::int64_t>(raw_scores.size())) {
      doc.doc_score = raw_scores[static_cast<std::size_t>(pred)];
    }
  }
}

std::vector<std::string> TextCatStage::dump_state_lines() const {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{
      std::string("lowercase=") + (options_.lowercase ? "1" : "0"),
      std::string("use_lemmas=") + (options_.use_lemmas ? "1" : "0"),
      std::string("skip_punct=") + (options_.skip_punct ? "1" : "0"),
      "max_features=" + std::to_string(options_.max_features),
      "min_document_frequency=" + std::to_string(options_.min_document_frequency),
      "max_iterations=" + std::to_string(options_.max_iterations),
      "tolerance=" + std::to_string(options_.tolerance),
      std::string("multi_label=") + (options_.multi_label ? "1" : "0"),
      "multi_label_threshold=" + std::to_string(options_.multi_label_threshold),
      "backend=" + std::string(detail::to_string(options_.backend)),
      "sequence_head=" + std::string(detail::to_string(options_.sequence_head)),
      "sequence_pooling=" + std::string(detail::to_string(options_.sequence_pooling)),
      "sequence_width=" + std::to_string(options_.sequence_width),
      "sequence_learning_rate=" + std::to_string(options_.sequence_learning_rate),
      "sequence_dropout=" + std::to_string(options_.sequence_dropout),
      "sequence_seed=" + std::to_string(options_.sequence_seed),
      "quantization=" + std::string(detail::to_string(options_.quantization)),
      "distillation=" + std::string(detail::to_string(options_.distillation)),
      std::string("trained=") + ((impl_ && impl_->trained) ? "1" : "0"),
      std::string("multiclass=") + ((impl_ && impl_->multiclass) ? "1" : "0"),
  };
  if (!impl_ || !impl_->trained) {
    lines.push_back("label_count=0");
    lines.push_back("mapper_vocab_count=0");
    lines.push_back("sequence_vocab_count=0");
    lines.push_back("sequence_cls_bias=");
    lines.push_back("sequence_cls_weight_count=0");
    return lines;
  }

  const auto mapper_state = impl_->mapper.dump_state();
  lines.push_back("label_count=" + std::to_string(impl_->label_values.size()));
  for (std::size_t i = 0; i < impl_->label_values.size(); ++i) {
    lines.push_back("label_" + std::to_string(i) + "=" + detail::hex_encode_string(impl_->label_values[i]));
  }

  lines.push_back("sequence_vocab_count=" + std::to_string(impl_->sequence_vocabulary.size()));
  for (std::size_t i = 0; i < impl_->sequence_vocabulary.size(); ++i) {
    lines.push_back("sequence_vocab_" + std::to_string(i) + "=" + detail::hex_encode_string(impl_->sequence_vocabulary[i]));
    lines.push_back("sequence_embed_" + std::to_string(i) + "=" + detail::join_doubles_line(impl_->sequence_embedding[i]));
  }
  lines.push_back("sequence_cls_bias=" + detail::join_doubles_line(impl_->sequence_cls_bias));
  lines.push_back("sequence_cls_weight_count=" + std::to_string(impl_->sequence_cls_weight.size()));
  for (std::size_t i = 0; i < impl_->sequence_cls_weight.size(); ++i) {
    lines.push_back("sequence_cls_weight_" + std::to_string(i) + "=" + detail::join_doubles_line(impl_->sequence_cls_weight[i]));
  }

  if (options_.backend == TaskBackend::Sequence) {
    lines.push_back("mapper_vocab_count=0");
    lines.push_back("model_state_count=0");
    return lines;
  }

  lines.push_back(std::string("mapper_lowercase=") + (mapper_state.options.lowercase ? "1" : "0"));
  lines.push_back("mapper_max_features=" + std::to_string(mapper_state.options.max_features));
  lines.push_back("mapper_min_document_frequency=" + std::to_string(mapper_state.options.min_document_frequency));
  lines.push_back(std::string("mapper_binary=") + (mapper_state.options.binary ? "1" : "0"));
  lines.push_back(std::string("mapper_learned=") + (mapper_state.learned ? "1" : "0"));
  lines.push_back("mapper_doc_count=" + std::to_string(mapper_state.learned_document_count));
  lines.push_back("mapper_vocab_count=" + std::to_string(mapper_state.vocabulary.size()));
  lines.push_back("mapper_df=" + join_numbers(mapper_state.document_frequency));
  for (std::size_t i = 0; i < mapper_state.vocabulary.size(); ++i) {
    lines.push_back("mapper_vocab_" + std::to_string(i) + "=" + detail::hex_encode_string(mapper_state.vocabulary[i]));
  }

  const auto state_or = impl_->multiclass ? impl_->multi.dump_state()
                                          : impl_->binary.dump_state();
  if (!state_or.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::dump_state_lines: " + state_or.status().message);
  }
  const auto& state = state_or.value();
  lines.push_back("model_state_count=" + std::to_string(state.size()));
  for (std::size_t i = 0; i < state.size(); ++i) {
    const auto values = state[i].to(pluss::pCPU, pluss::ScalarType::Float64)
                            .reshape({state[i].numel()})
                            .to_vec<double>();
    lines.push_back("model_state_" + std::to_string(i) + "=" + detail::join_doubles_line(values));
  }
  return lines;
}

void TextCatStage::load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) {
  PLUSS_CALL_GUARD();
  options_ = {};
  impl_ = std::make_shared<Impl>();

  std::int64_t label_count = -1;
  std::vector<std::string> labels{};
  pluss::ai::nodej::LexiconMapperState mapper_state{};
  std::int64_t mapper_vocab_count = -1;

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::load_state_lines: expected key=value line.");
    }
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == "lowercase") { options_.lowercase = value == "1"; continue; }
    if (key == "use_lemmas") { options_.use_lemmas = value == "1"; continue; }
    if (key == "skip_punct") { options_.skip_punct = value == "1"; continue; }
    if (key == "multi_label") { options_.multi_label = value == "1"; continue; }
    if (key == "multi_label_threshold") { options_.multi_label_threshold = std::stod(std::string(value)); continue; }
    if (key == "max_features") { options_.max_features = std::stoll(std::string(value)); continue; }
    if (key == "min_document_frequency") { options_.min_document_frequency = std::stoll(std::string(value)); continue; }
    if (key == "max_iterations") { options_.max_iterations = std::stoll(std::string(value)); continue; }
    if (key == "tolerance") { options_.tolerance = std::stod(std::string(value)); continue; }
    if (key == "backend") { options_.backend = detail::parse_task_backend(value); continue; }
    if (key == "sequence_head") { options_.sequence_head = detail::parse_sequence_head(value); continue; }
    if (key == "sequence_pooling") { options_.sequence_pooling = detail::parse_sequence_pooling(value); continue; }
    if (key == "sequence_width") { options_.sequence_width = std::stoll(std::string(value)); continue; }
    if (key == "sequence_learning_rate") { options_.sequence_learning_rate = std::stod(std::string(value)); continue; }
    if (key == "sequence_dropout") { options_.sequence_dropout = std::stod(std::string(value)); continue; }
    if (key == "sequence_seed") { options_.sequence_seed = static_cast<std::uint64_t>(std::stoull(std::string(value))); continue; }
    if (key == "quantization") { options_.quantization = detail::parse_quantization_hint(value); continue; }
    if (key == "distillation") { options_.distillation = detail::parse_distillation_hint(value); continue; }
    if (key == "trained") { impl_->trained = value == "1"; continue; }
    if (key == "multiclass") { impl_->multiclass = value == "1"; continue; }
    if (key == "label_count") {
      label_count = std::stoll(std::string(value));
      labels.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, label_count)), {});
      continue;
    }
    if (key.rfind("label_", 0) == 0 &&
        std::all_of(key.begin() + 6, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(6)));
      if (idx >= labels.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: label index out of range.");
      }
      labels[idx] = detail::hex_decode_string(value);
      continue;
    }
    if (key == "mapper_lowercase") { mapper_state.options.lowercase = value == "1"; continue; }
    if (key == "mapper_max_features") { mapper_state.options.max_features = std::stoll(std::string(value)); continue; }
    if (key == "mapper_min_document_frequency") { mapper_state.options.min_document_frequency = std::stoll(std::string(value)); continue; }
    if (key == "mapper_binary") { mapper_state.options.binary = value == "1"; continue; }
    if (key == "mapper_learned") { mapper_state.learned = value == "1"; continue; }
    if (key == "mapper_doc_count") { mapper_state.learned_document_count = std::stoll(std::string(value)); continue; }
    if (key == "mapper_vocab_count") {
      mapper_vocab_count = std::stoll(std::string(value));
      mapper_state.vocabulary.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, mapper_vocab_count)), {});
      continue;
    }
    if (key == "mapper_df") {
      const auto parsed = parse_i64s(value);
      mapper_state.document_frequency = std::move(parsed);
      continue;
    }
    if (key == "sequence_vocab_count") {
      const auto count = std::stoll(std::string(value));
      impl_->sequence_vocabulary.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, count)), {});
      impl_->sequence_embedding.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, count)), {});
      continue;
    }
    if (key.rfind("sequence_vocab_", 0) == 0 &&
        std::all_of(key.begin() + 15, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(15)));
      if (idx >= impl_->sequence_vocabulary.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: sequence vocabulary index out of range.");
      }
      impl_->sequence_vocabulary[idx] = detail::hex_decode_string(value);
      continue;
    }
    if (key.rfind("sequence_embed_", 0) == 0 &&
        std::all_of(key.begin() + 15, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(15)));
      if (idx >= impl_->sequence_embedding.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: sequence embedding index out of range.");
      }
      impl_->sequence_embedding[idx] = detail::parse_doubles_line(value);
      continue;
    }
    if (key == "sequence_cls_bias") {
      impl_->sequence_cls_bias = detail::parse_doubles_line(value);
      continue;
    }
    if (key == "sequence_cls_weight_count") {
      const auto count = std::stoll(std::string(value));
      impl_->sequence_cls_weight.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, count)), {});
      continue;
    }
    if (key.rfind("sequence_cls_weight_", 0) == 0 &&
        std::all_of(key.begin() + 20, key.end(), [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(20)));
      if (idx >= impl_->sequence_cls_weight.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: sequence classifier weight index out of range.");
      }
      impl_->sequence_cls_weight[idx] = detail::parse_doubles_line(value);
      continue;
    }
    constexpr std::size_t mapper_vocab_prefix_len = 13;
    if (key.rfind("mapper_vocab_", 0) == 0 &&
        std::all_of(key.begin() + static_cast<std::ptrdiff_t>(mapper_vocab_prefix_len),
                    key.end(),
                    [](char ch) { return ch >= '0' && ch <= '9'; })) {
      const auto idx = static_cast<std::size_t>(std::stoll(key.substr(mapper_vocab_prefix_len)));
      if (idx >= mapper_state.vocabulary.size()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: mapper vocabulary index out of range.");
      }
      mapper_state.vocabulary[idx] = detail::hex_decode_string(value);
      continue;
    }
  }

  impl_->label_values = std::move(labels);
  impl_->sequence_token_to_id.clear();
  impl_->sequence_token_to_id.reserve(impl_->sequence_vocabulary.size());
  for (std::size_t i = 0; i < impl_->sequence_vocabulary.size(); ++i) {
    impl_->sequence_token_to_id.emplace(impl_->sequence_vocabulary[i], static_cast<std::int64_t>(i));
  }
  if (options_.backend == TaskBackend::Sequence) {
    const auto width = static_cast<std::size_t>(std::max<std::int64_t>(0, options_.sequence_width));
    const bool shapes_consistent =
        !impl_->sequence_vocabulary.empty() &&
        impl_->sequence_embedding.size() == impl_->sequence_vocabulary.size() &&
        impl_->sequence_cls_weight.size() == width && !impl_->sequence_cls_weight.empty() &&
        impl_->sequence_cls_bias.size() == impl_->sequence_cls_weight.front().size() &&
        std::ranges::all_of(impl_->sequence_embedding, [width](const auto& row) { return row.size() == width; }) &&
        std::ranges::all_of(impl_->sequence_cls_weight, [this](const auto& row) {
          return row.size() == impl_->sequence_cls_bias.size();
        });
    if (impl_->trained && impl_->label_values.size() >= 2 && shapes_consistent) {
      return;
    }
    if (!impl_->trained) return;
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::TextCatStage::load_state_lines: malformed sequence textcat state.");
  }
  if (impl_->trained) {
    if (label_count < 2) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::load_state_lines: trained state requires at least two labels.");
    }
    if (mapper_vocab_count < 1) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::load_state_lines: trained state requires mapper vocabulary.");
    }
    const auto mapper_status = impl_->mapper.load_state(mapper_state);
    if (!mapper_status.ok()) {
      ::pluss::ai::nodeb::detail::fail(
          "nodeb::TextCatStage::load_state_lines: " + mapper_status.message);
    }
    const auto state = parse_series_state_lines("model_state", lines);
    if (impl_->multiclass) {
      const auto status = impl_->multi.load_state(pluss::msharp::RangeView::Series(state));
      if (!status.ok()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: " + status.message);
      }
    } else {
      const auto status = impl_->binary.load_state(pluss::msharp::RangeView::Series(state));
      if (!status.ok()) {
        ::pluss::ai::nodeb::detail::fail(
            "nodeb::TextCatStage::load_state_lines: " + status.message);
      }
    }
  }
}

namespace stage {

Stage textcat(TextCatOptions options) {
  return std::make_shared<TextCatStage>(options);
}

} // namespace stage

} // namespace pluss::ai::nodeb
