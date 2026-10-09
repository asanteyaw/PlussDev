#include <pluss/nlp.h>

#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

namespace nlp = pluss::ai::nlp;

void print_strings(std::string_view label,
                   const std::vector<std::string>& values) {
  std::cout << label << "=[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << std::quoted(values[i]);
  }
  std::cout << "]\n";
}

void print_spans(std::string_view label,
                 const nlp::Doc& doc,
                 const std::vector<nlp::Span>& spans) {
  std::cout << label << "=[";
  for (std::size_t i = 0; i < spans.size(); ++i) {
    if (i != 0) std::cout << ", ";
    std::cout << spans[i].label << ':' << std::quoted(spans[i].text(doc));
  }
  std::cout << "]\n";
}

void print_error(std::string_view label, const auto& call) {
  try {
    call();
  } catch (const std::exception& error) {
    const std::string_view message(error.what());
    std::cout << label << '=' << message.substr(0, message.find('\n')) << '\n';
  }
}

nlp::Doc prepared(std::string text) {
  nlp::Workflow prep{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()},
  };
  return prep.infer(text);
}

nlp::Doc gold(std::string text,
              std::vector<std::string> tags,
              std::vector<nlp::Span> entities = {}) {
  auto doc = prepared(std::move(text));
  for (std::size_t i = 0; i < tags.size(); ++i) {
    doc.tokens[i].pos = std::move(tags[i]);
  }
  doc.entities = std::move(entities);
  return doc;
}

} // namespace

int main() {
  std::cout << std::boolalpha;

  nlp::Workflow token_default{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize()},
  };
  const auto tokenized = token_default.infer("One, two! Email help@pluss.dev.");
  std::cout << "tokenizer\n";
  print_strings("default", tokenized.token_texts());
  nlp::Workflow token_words{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize({.keep_punct = false})},
  };
  print_strings("without_punct",
                token_words.infer("One, two! Email help@pluss.dev.").token_texts());
  nlp::RuleTokenizerStage token_stage({.keep_punct = false,
                                       .use_language_exceptions = false});
  print_strings("state", token_stage.dump_state_lines());

  nlp::Workflow sentences{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::sentencize()},
  };
  std::cout << "sentencizer\n";
  print_strings("default",
                sentences.infer("Dr. Ada writes.\nTests pass!").sentence_texts());
  nlp::Workflow newline_only{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(),
                 nlp::stage::sentencize({.break_on_punct = false})},
  };
  print_strings("newline_only",
                newline_only.infer("First. Still first.\nSecond!").sentence_texts());

  std::cout << "lemmatizer\n";
  nlp::Workflow lemmas{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize()},
  };
  print_strings("default", lemmas.infer("Mice wrote tests.").lemma_texts());
  nlp::Workflow surfaces{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(),
                 nlp::stage::lemmatize({.lowercase = false})},
  };
  print_strings("preserve_surface", surfaces.infer("Mice wrote tests.").lemma_texts());

  std::cout << "matcher\n";
  nlp::Workflow matcher{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::lemmatize(),
          nlp::stage::matcher({
              nlp::MatcherPattern{.label = "ACTION",
                                  .tokens = {"write", "test"},
                                  .use_lemmas = true},
              nlp::MatcherPattern{.label = "PRODUCT",
                                  .tokens = {"Pluss", "SDK"},
                                  .use_lemmas = false},
          }),
      },
  };
  const auto matched = matcher.infer("They write tests for the Pluss SDK.");
  print_spans("matches", matched, matched.matches);
  nlp::MatcherStage growing;
  growing.add_pattern({.label = "PAIR", .tokens = {"go", "go"}, .use_lemmas = false});
  auto repeated = prepared("go go go");
  growing.apply(repeated);
  print_spans("overlap", repeated, repeated.matches);

  std::cout << "tagger\n";
  nlp::Workflow heuristic_tags{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::tagger()},
  };
  const auto tagged = heuristic_tags.infer("Ada writes 42 tests.");
  std::vector<std::string> tagged_pairs;
  for (const auto& token : tagged.tokens) {
    tagged_pairs.push_back(token.orth + '/' + token.pos);
  }
  print_strings("heuristic", tagged_pairs);

  std::cout << "syntax\n";
  nlp::Workflow syntax{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::tagger(), nlp::stage::syntax()},
  };
  const auto parsed = syntax.infer("Ada writes tests.");
  for (const auto& token : parsed.tokens) {
    std::cout << token.orth << " head=" << token.chief
              << " role=" << token.role << '\n';
  }
  print_spans("phrases", parsed, parsed.phrases);

  std::cout << "ner\n";
  nlp::Workflow entities{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(), nlp::stage::lemmatize(),
          nlp::stage::ner({nlp::EntityPattern{
              .label = "GPE", .tokens = {"New", "York"}, .use_lemmas = false}}),
      },
  };
  const auto recognized = entities.infer("Dr. Ada moved to New York for NASA.");
  print_spans("entities", recognized, recognized.entities);

  const std::vector<nlp::Doc> training{
      gold("Ada writes tests.", {"PROPN", "VERB", "NOUN", "PUNCT"},
           {{.token_start = 0, .token_end = 1, .label = "PERSON"}}),
      gold("Kofi writes APIs.", {"PROPN", "VERB", "NOUN", "PUNCT"},
           {{.token_start = 0, .token_end = 1, .label = "PERSON"}}),
  };
  nlp::Workflow learned{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(), nlp::stage::lemmatize(),
          nlp::stage::tagger({.use_heuristics = false}),
          nlp::stage::ner({}, {.use_heuristics = false}),
      },
  };
  learned.learn(training);
  const auto learned_docs = learned.infer(
      std::vector<std::string>{"Ada writes tests.", "Kofi writes APIs."});
  const auto tag_score = nlp::assess::tagger(learned_docs, training);
  const auto entity_score = nlp::assess::ner(learned_docs, training);
  std::cout << "learned_and_scores\n"
            << "tag docs=" << tag_score.doc_count
            << " tokens=" << tag_score.token_count
            << " correct=" << tag_score.correct_count
            << " accuracy=" << tag_score.accuracy << '\n'
            << "ner docs=" << entity_score.doc_count
            << " gold=" << entity_score.gold_count
            << " predicted=" << entity_score.predicted_count
            << " correct=" << entity_score.correct_count
            << " precision=" << entity_score.precision
            << " recall=" << entity_score.recall
            << " f1=" << entity_score.f1 << '\n';

  nlp::Workflow classifier{
      .lang = nlp::Language::English,
      .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(),
                 nlp::stage::textcat()},
  };
  const std::vector<std::string> class_texts{
      "compiler tests pass", "compiler builds pass",
      "coastal flights depart", "coastal holidays begin",
  };
  const std::vector<std::string> class_labels{"tech", "tech", "travel", "travel"};
  classifier.learn(class_texts, class_labels);
  const auto classified = classifier.infer(class_texts);
  std::cout << "textcat\n";
  for (const auto& doc : classified) {
    std::cout << std::quoted(doc.text) << " -> " << doc.doc_label
              << " confident=" << (doc.doc_score >= 0.5) << '\n';
  }

  std::cout << "failures\n";
  print_error("matcher_pattern", [] {
    nlp::MatcherStage bad({nlp::MatcherPattern{.label = "", .tokens = {"x"}}});
  });
  print_error("entity_pattern", [] {
    nlp::NerStage bad({nlp::EntityPattern{.label = "X", .tokens = {}}});
  });
  print_error("tag_alignment", [&] {
    static_cast<void>(nlp::assess::tagger(learned_docs,
                                          std::vector<nlp::Doc>{training.front()}));
  });
  print_error("textcat_labels", [&] {
    nlp::TextCatStage bad;
    bad.learn(training,
              pluss::msharp::RangeView::Of<std::string_view>(
                  std::vector<std::string_view>{"one"}),
              {});
  });
}
