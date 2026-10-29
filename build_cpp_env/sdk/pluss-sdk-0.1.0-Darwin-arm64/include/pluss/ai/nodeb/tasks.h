#pragma once

#include "pluss/ai/nodeb/workflow.h"

namespace pluss::ai::nodeb {

struct MatcherPattern {
  std::string label{};
  std::vector<std::string> tokens{};
  bool use_lemmas = true;
};

struct EntityPattern {
  std::string label{};
  std::vector<std::string> tokens{};
  bool use_lemmas = true;
};

class SentencizerStage final : public IStage {
public:
  explicit SentencizerStage(SentencizerOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

private:
  SentencizerOptions options_{};
};

class LemmatizerStage final : public IStage {
public:
  explicit LemmatizerStage(LemmatizerOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

private:
  LemmatizerOptions options_{};
};

class MatcherStage final : public IStage {
public:
  explicit MatcherStage(std::vector<MatcherPattern> patterns = {},
                        MatcherOptions options = {});
  MatcherStage(std::initializer_list<MatcherPattern> patterns,
               MatcherOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

  MatcherStage& add_pattern(MatcherPattern pattern);

private:
  std::vector<MatcherPattern> patterns_{};
  MatcherOptions options_{};
};

class TaggerStage final : public IStage {
public:
  explicit TaggerStage(TaggerOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  void learn(const std::vector<Doc>& docs,
             const TrainOptions& options) override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;
  [[nodiscard]] TrainingReport training_report() const;

private:
  struct Impl;
  std::shared_ptr<Impl> impl_{};
  TaggerOptions options_{};
};

class SyntaxStage final : public IStage {
public:
  explicit SyntaxStage(SyntaxOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

private:
  SyntaxOptions options_{};
};

class NerStage final : public IStage {
public:
  explicit NerStage(std::vector<EntityPattern> patterns = {},
                    NerOptions options = {});
  NerStage(std::initializer_list<EntityPattern> patterns,
           NerOptions options = {});

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  void learn(const std::vector<Doc>& docs,
             const TrainOptions& options) override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;

  NerStage& add_pattern(EntityPattern pattern);
  [[nodiscard]] TrainingReport training_report() const;

private:
  struct Impl;
  std::shared_ptr<Impl> impl_{};
  std::vector<EntityPattern> patterns_{};
  NerOptions options_{};
};

class TextCatStage final : public IStage {
public:
  explicit TextCatStage(TextCatOptions options = {});
  ~TextCatStage() override;

  [[nodiscard]] std::string_view name() const noexcept override;
  void apply(Doc& doc) const override;
  void learn(const std::vector<Doc>& docs,
             pluss::msharp::RangeView::Of<std::string_view> labels,
             const TrainOptions& options) override;
  [[nodiscard]] std::vector<std::string> dump_state_lines() const override;
  void load_state_lines(pluss::msharp::RangeView::Of<std::string_view> lines) override;
  [[nodiscard]] TrainingReport training_report() const;

private:
  struct Impl;
  std::shared_ptr<Impl> impl_{};
  TextCatOptions options_{};
};

namespace stage {

[[nodiscard]] Stage sentencize(SentencizerOptions options = {});
[[nodiscard]] Stage lemmatize(LemmatizerOptions options = {});
[[nodiscard]] Stage matcher(std::vector<MatcherPattern> patterns = {},
                            MatcherOptions options = {});
[[nodiscard]] Stage matcher(std::initializer_list<MatcherPattern> patterns,
                            MatcherOptions options = {});
[[nodiscard]] Stage tagger(TaggerOptions options = {});
[[nodiscard]] Stage syntax(SyntaxOptions options = {});
[[nodiscard]] Stage ner(std::vector<EntityPattern> patterns = {},
                        NerOptions options = {});
[[nodiscard]] Stage ner(std::initializer_list<EntityPattern> patterns,
                        NerOptions options = {});
[[nodiscard]] Stage textcat(TextCatOptions options = {});

} // namespace stage

struct TagScore {
  std::int64_t doc_count = 0;
  std::int64_t token_count = 0;
  std::int64_t correct_count = 0;
  double accuracy = 0.0;
};

struct EntityScore {
  std::int64_t doc_count = 0;
  std::int64_t gold_count = 0;
  std::int64_t predicted_count = 0;
  std::int64_t correct_count = 0;
  double precision = 0.0;
  double recall = 0.0;
  double f1 = 0.0;
};

namespace assess {

[[nodiscard]] TagScore tagger(const std::vector<Doc>& predicted,
                              const std::vector<Doc>& gold);
[[nodiscard]] EntityScore ner(const std::vector<Doc>& predicted,
                              const std::vector<Doc>& gold);

} // namespace assess

} // namespace pluss::ai::nodeb
