#include <pluss/nlp.h>

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace nlp = pluss::ai::nlp;

class MarkerStage final : public nlp::IStage {
public:
  std::string_view name() const noexcept override { return "marker"; }

  void apply(nlp::Doc& doc) const override {
    doc.doc_label = label_;
  }

  void learn(const std::vector<nlp::Doc>& docs,
             const nlp::TrainOptions&) override {
    label_ = "docs=" + std::to_string(docs.size());
  }

  void learn(const std::vector<nlp::Doc>& docs,
             pluss::msharp::RangeView::Of<std::string_view> labels,
             const nlp::TrainOptions&) override {
    label_ = "docs=" + std::to_string(docs.size()) +
             ",labels=" + std::to_string(labels.size()) +
             ",first=" + (labels.empty() ? std::string{} : std::string(labels[0]));
  }

  std::vector<std::string> dump_state_lines() const override {
    return {"label=" + label_};
  }

  void load_state_lines(
      pluss::msharp::RangeView::Of<std::string_view> lines) override {
    if (lines.size() != 1 || !lines[0].starts_with("label=")) {
      throw std::invalid_argument("MarkerStage: expected one label line");
    }
    label_ = std::string(lines[0].substr(6));
  }

private:
  std::string label_ = "untrained";
};

void print_error(std::string_view label, const auto& call) {
  try {
    call();
  } catch (const std::exception& error) {
    const std::string_view message(error.what());
    std::cout << label << '=' << message.substr(0, message.find('\n')) << '\n';
  }
}

} // namespace

int main() {
  nlp::TextPool pool;
  const auto alpha = pool.intern("alpha");
  const auto alpha_again = pool.intern("alpha");
  const auto beta = pool.intern("beta");
  std::cout << "text_pool\n"
            << "empty=" << pool.intern("")
            << " stable=" << std::boolalpha << (alpha == alpha_again)
            << " distinct=" << (alpha != beta)
            << " size=" << pool.size() << '\n'
            << "lookup=" << (pool.lookup("alpha") == alpha)
            << " absent=" << pool.lookup("gamma")
            << " value=" << pool.at(alpha) << '\n';
  print_error("zero_error", [&] { static_cast<void>(pool.at(0)); });
  pool.clear();
  std::cout << "after_clear=" << pool.size()
            << " contains_old=" << pool.contains(alpha) << '\n';

  nlp::Workflow tokenizer{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()},
  };
  const auto doc = tokenizer.infer("Laurel writes APIs; Laurel tests 42.");
  const auto* laurel = doc.lexicon->find("Laurel");
  const auto* writes = doc.lexicon->find("writes");
  const auto* tests = doc.lexicon->find("tests");
  std::cout << "lexicon\n"
            << "lexemes=" << doc.lexicon->size()
            << " pooled=" << doc.lexicon->texts().size()
            << " morphs=" << doc.lexicon->morphs().size() << '\n'
            << "laurel=" << laurel->lower
            << " lemma=" << laurel->lemma
            << " shape=" << laurel->morphology.shape
            << " prefix=" << laurel->prefix
            << " suffix=" << laurel->suffix << '\n'
            << "repeat_id=" << (doc.tokens[0].orth_id == doc.tokens[4].orth_id)
            << " class_id=" << (writes->morphology_id == tests->morphology_id)
            << " case_sensitive=" << (doc.lexicon->find("laurel") == nullptr) << '\n';

  const auto& token = doc.tokens.front();
  std::cout << "token\n"
            << "index=" << token.index
            << " chars=[" << token.char_start << ',' << token.char_end << ")"
            << " orth=" << token.orth
            << " lower=" << doc.lexicon->text(token.lower_id)
            << " lemma=" << token.lemma
            << " punct=" << token.is_punct << '\n'
            << "kind=" << static_cast<int>(token.morphology.kind)
            << " case=" << static_cast<int>(token.morphology.casing)
            << " length=" << token.morphology.length
            << " alpha=" << token.morphology.has_alpha
            << " shape=" << token.morphology.shape
            << " norm=" << token.morphology.norm << '\n';

  nlp::TextPool morphology_texts;
  nlp::MorphBank bank(&morphology_texts);
  nlp::Morphology first{
      .casing = nlp::CaseKind::Lower,
      .kind = nlp::TokenKind::Word,
      .length = 5,
      .is_ascii = true,
      .has_alpha = true,
      .is_lowercase = true,
      .norm = "alpha",
      .shape = "xxxxx",
      .spec = "lower-word",
  };
  auto second = first;
  second.norm = "bravo";
  const auto first_id = bank.intern(first);
  const auto second_id = bank.intern(second);
  std::cout << "morph_bank\n"
            << "same_class=" << (first_id == second_id)
            << " classes=" << bank.size()
            << " first_norm=" << bank.find(first_id)->norm
            << " absent=" << (bank.find(0) == nullptr) << '\n';
  nlp::MorphBank detached;
  print_error("attach_error", [&] { static_cast<void>(detached.intern(first)); });

  nlp::Lexicon direct_lexicon;
  const auto raw_alpha = direct_lexicon.intern("Alpha");
  const auto raw_alpha_again = direct_lexicon.intern("Alpha");
  std::cout << "lexicon_intern\n"
            << "empty=" << direct_lexicon.intern("")
            << " stable=" << (raw_alpha == raw_alpha_again)
            << " lookup=" << (direct_lexicon.lookup("Alpha") == raw_alpha)
            << " absent=" << direct_lexicon.lookup("missing")
            << " lexemes=" << direct_lexicon.size()
            << " texts=" << direct_lexicon.texts().size() << '\n'
            << "text=" << direct_lexicon.text(raw_alpha)
            << " find_raw=" << (direct_lexicon.find(raw_alpha) != nullptr) << '\n';
  const auto& direct_title = direct_lexicon.observe("Laurel");
  const auto title_id = direct_title.id;
  const auto norm_id = direct_title.norm_id;
  const auto& direct_repeat = direct_lexicon.observe("Laurel");
  std::cout << "lexeme_repeat\n"
            << "same_id=" << (title_id == direct_repeat.id)
            << " lexemes=" << direct_lexicon.size() << '\n';
  const auto& direct_lower = direct_lexicon.observe("laurel");
  std::cout << "lexeme_case\n"
            << "same_lexeme=" << (title_id == direct_lower.id)
            << " same_norm=" << (norm_id == direct_lower.norm_id)
            << " lexemes=" << direct_lexicon.size() << '\n';
  std::cout << "lexicon_find\n"
            << "by_text=" << direct_lexicon.find("Laurel")->text
            << " by_id=" << direct_lexicon.find(title_id)->text
            << " missing=" << (direct_lexicon.find("missing") == nullptr)
            << " raw_only=" << (direct_lexicon.find(raw_alpha) == nullptr) << '\n'
            << "texts=" << direct_lexicon.texts().size()
            << " morphs=" << direct_lexicon.morphs().size()
            << " lexemes=" << direct_lexicon.size() << '\n';
  const auto* direct_morph =
      direct_lexicon.find_morphology(direct_title.morphology_id);
  std::cout << "lexicon_morph\n"
            << "found=" << (direct_morph != nullptr)
            << " shape=" << direct_morph->shape
            << " absent=" << (direct_lexicon.find_morphology(0) == nullptr)
            << '\n';
  print_error("observe_error",
              [&] { static_cast<void>(direct_lexicon.observe("")); });
  print_error("morph_error", [&] {
    static_cast<void>(direct_lexicon.intern_morphology(nlp::Morphology{}));
  });
  const auto old_title_id = direct_title.id;
  direct_lexicon.clear();
  std::cout << "lexicon_clear\n"
            << "lexemes=" << direct_lexicon.size()
            << " texts=" << direct_lexicon.texts().size()
            << " morphs=" << direct_lexicon.morphs().size()
            << " old_lookup=" << direct_lexicon.lookup("Laurel")
            << " old_find=" << (direct_lexicon.find(old_title_id) != nullptr)
            << '\n';
  print_error("text_error",
              [&] { static_cast<void>(direct_lexicon.text(old_title_id)); });

  nlp::Lexicon derived;
  static_cast<void>(derived.observe("Laurel"));
  std::cout << "lexicon_derived\n"
            << "lower_id=" << (derived.lookup("laurel") != 0)
            << " lower_lexeme=" << (derived.find("laurel") != nullptr)
            << " title_lexeme=" << (derived.find("Laurel") != nullptr)
            << " texts=" << derived.texts().size() << '\n';

  nlp::Lexicon counted;
  static_cast<void>(counted.intern("metadata"));
  static_cast<void>(counted.observe("writes"));
  static_cast<void>(counted.observe("tests"));
  std::cout << "lexicon_counts\n"
            << "lexemes=" << counted.size()
            << " texts=" << counted.texts().size()
            << " morphs=" << counted.morphs().size() << '\n';
  print_error("zero_text_error",
              [&] { static_cast<void>(counted.text(nlp::StringId{0})); });
  print_error("unknown_text_error",
              [&] { static_cast<void>(counted.text(nlp::StringId{42})); });

  const auto kinds = tokenizer.infer("42 help@pluss.dev !");
  std::cout << "morph_kinds\n";
  for (const auto& item : kinds.tokens) {
    std::cout << item.orth << ':'
              << item.morphology.is_number_like << ','
              << item.morphology.is_email << ','
              << item.morphology.is_punct << '\n';
  }
  const auto classes = tokenizer.infer("writes tests");
  std::cout << "morph_class\n"
            << "same_spelling="
            << (classes.tokens[0].orth_id == classes.tokens[1].orth_id)
            << " same_class="
            << (classes.tokens[0].morphology_id == classes.tokens[1].morphology_id)
            << " spec=" << classes.tokens[0].morphology.spec << '\n';

  nlp::Workflow tagged{
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::sentencize(),
          nlp::stage::lemmatize(),
          nlp::stage::tagger(),
      },
  };
  const auto tagged_doc = tagged.infer("Laurel writes.");
  std::cout << "tagged_tokens\n";
  for (const auto& item : tagged_doc.tokens) {
    std::cout << item.index << ':' << item.orth
              << " lemma=" << item.lemma
              << " pos=" << item.pos
              << " start=" << item.sentence_start
              << " end=" << item.sentence_end << '\n';
  }
  const auto repeats = tokenizer.infer("Laurel met Laurel.");
  std::cout << "token_repeat\n"
            << "same_orth_id="
            << (repeats.tokens[0].orth_id == repeats.tokens[2].orth_id)
            << " first_index=" << repeats.tokens[0].index
            << " second_index=" << repeats.tokens[2].index
            << " first_start=" << repeats.tokens[0].char_start
            << " second_start=" << repeats.tokens[2].char_start << '\n';

  nlp::Doc spaced = tokenizer.infer("red   green blue");
  const nlp::Span middle{.token_start = 0, .token_end = 2, .label = "PAIR"};
  const nlp::Span empty{.token_start = 1, .token_end = 1};
  const nlp::Span invalid{.token_start = -1, .token_end = 1};
  std::cout << "span\n"
            << "text='" << middle.text(spaced) << "' label=" << middle.label
            << " empty=" << empty.empty()
            << " empty_text='" << empty.text(spaced) << "'"
            << " invalid_text='" << invalid.text(spaced) << "'\n";

  auto marker = std::make_shared<MarkerStage>();
  marker->learn({nlp::Doc{}, nlp::Doc{}}, nlp::TrainOptions{});
  nlp::Doc marked;
  marker->apply(marked);
  const auto state = marker->dump_state_lines();
  auto restored = std::make_shared<MarkerStage>();
  std::vector<std::string_view> state_view{state.front()};
  restored->load_state_lines(state_view);
  nlp::Doc roundtrip;
  restored->apply(roundtrip);
  std::cout << "stage\n"
            << "name=" << marker->name()
            << " label=" << marked.doc_label
            << " state=" << state.front()
            << " restored=" << roundtrip.doc_label << '\n';

  std::vector<std::string_view> labels{"positive", "negative"};
  marker->learn({nlp::Doc{}, nlp::Doc{}}, labels, nlp::TrainOptions{});
  nlp::Doc labeled;
  marker->apply(labeled);
  std::cout << "stage_labeled\n"
            << labeled.doc_label << '\n';

  nlp::Workflow custom{.stages = {marker}};
  const auto custom_doc = custom.infer("source text");
  std::cout << "stage_workflow\n"
            << "text=" << custom_doc.text
            << " label=" << custom_doc.doc_label << '\n';

  class StatelessStage final : public nlp::IStage {
  public:
    std::string_view name() const noexcept override { return "stateless"; }
    void apply(nlp::Doc&) const override {}
  } stateless;
  stateless.learn({nlp::Doc{}}, nlp::TrainOptions{});
  std::vector<std::string_view> no_labels{};
  stateless.learn({nlp::Doc{}}, no_labels, nlp::TrainOptions{});
  stateless.load_state_lines(no_labels);
  std::cout << "stage_defaults\n"
            << "name=" << stateless.name()
            << " state_lines=" << stateless.dump_state_lines().size()
            << " empty_load=accepted\n";
  std::cout << "stage_names\n";
  for (const auto& item : std::vector<nlp::Stage>{
           nlp::stage::tokenize(),
           nlp::stage::sentencize(),
           nlp::stage::lemmatize(),
       }) {
    std::cout << item->name() << '\n';
  }
  std::vector<std::string_view> unexpected{"state=1"};
  print_error("state_error", [&] { stateless.load_state_lines(unexpected); });
}
