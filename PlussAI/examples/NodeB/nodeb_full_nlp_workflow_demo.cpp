#include <pluss/nlp.h>

#include <filesystem>
#include <iostream>
#include <vector>

int main() {
  namespace nlp_api = pluss::ai::nlp;
  using namespace nlp_api;

  Workflow tokenizer{
      .lang = Language::English,
      .stages = {stage::tokenize()},
  };

  std::vector<std::string> train_texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 16; ++i) {
    train_texts.push_back("Laurel reviews compiler warnings at Acme");
    labels.push_back("tech");
    train_texts.push_back("Ama cooks dinner at Bistro");
    labels.push_back("food");
  }

  auto gold_docs = tokenizer.infer(train_texts);
  std::vector<std::string_view> label_views{};
  label_views.reserve(labels.size());
  for (const auto& label : labels) {
    label_views.push_back(label);
  }
  for (std::size_t i = 0; i < gold_docs.size(); ++i) {
    auto& doc = gold_docs[i];
    const bool tech = labels[i] == "tech";
    const std::size_t expected_tokens = tech ? 6 : 5;
    if (doc.tokens.size() != expected_tokens) {
      std::cerr << "unexpected tokenization shape in training corpus: "
                << doc.tokens.size() << " tokens\n";
      return 1;
    }

    doc.tokens[0].pos = "PROPN";
    doc.tokens[1].pos = "VERB";
    doc.tokens[2].pos = "NOUN";
    if (tech) {
      doc.tokens[3].pos = "NOUN";
      doc.tokens[4].pos = "ADP";
      doc.tokens[5].pos = "PROPN";
    } else {
      doc.tokens[3].pos = "ADP";
      doc.tokens[4].pos = "PROPN";
    }

    doc.entities.push_back(Span{
        .token_start = 0,
        .token_end = 1,
        .label = "PERSON",
    });
    doc.entities.push_back(Span{
        .token_start = static_cast<std::int64_t>(expected_tokens - 1),
        .token_end = static_cast<std::int64_t>(expected_tokens),
        .label = tech ? "ORG" : "FAC",
    });
  }

  Workflow nlp{
      .lang = Language::English,
      .stages = {
          stage::tokenize(),
          stage::sentencize(),
          stage::lemmatize(),
          std::make_shared<TaggerStage>(TaggerOptions{
              .prefer_learned = false,
              .use_heuristics = false,
              .backend = TaskBackend::Sequence,
              .sequence_width = 24,
              .sequence_seed = 7,
          }),
          std::make_shared<NerStage>(
              std::vector<EntityPattern>{},
              NerOptions{
                  .allow_overlap = false,
                  .use_heuristics = false,
                  .prefer_learned = false,
                  .backend = TaskBackend::Sequence,
                  .sequence_width = 24,
                  .sequence_max_span_width = 2,
                  .sequence_min_score = 0.0,
                  .sequence_seed = 9,
              }),
          stage::matcher({
              MatcherPattern{.label = "TECH_TOPIC", .tokens = {"compiler", "warnings"}, .use_lemmas = true},
              MatcherPattern{.label = "FOOD_TOPIC", .tokens = {"cook", "dinner"}, .use_lemmas = true},
          }),
          stage::syntax(),
          stage::textcat(TextCatOptions{
              .backend = TaskBackend::Sequence,
              .sequence_width = 32,
              .sequence_seed = 42,
          }),
      },
  };

  nlp.learn(
      gold_docs,
      pluss::msharp::RangeView::Of<std::string_view>(label_views),
      TrainOptions{
          .random = {.seed = 42, .deterministic = true},
          .allow_empty_docs = false,
      });

  const std::string probe = "Laurel reviews compiler warnings at Acme";
  const auto doc = nlp.infer(probe);

  std::cout << "== PlussAI NLP Full Workflow ==\n";
  std::cout << "text: " << probe << "\n";
  std::cout << "doc_label=" << doc.doc_label << " score=" << doc.doc_score << "\n\n";

  std::cout << "Tokens\n";
  for (const auto& token : doc.tokens) {
    std::cout << "  " << token.orth
              << " lemma=" << token.lemma
              << " pos=" << token.pos
              << " role=" << token.role;
    if (!token.entity_label.empty()) {
      std::cout << " entity=" << token.entity_label;
    }
    std::cout << "\n";
  }

  std::cout << "\nEntities\n";
  for (const auto& entity : doc.entities) {
    std::cout << "  " << entity.label << ": " << entity.text(doc) << "\n";
  }

  std::cout << "\nMatches\n";
  for (const auto& match : doc.matches) {
    std::cout << "  " << match.label << ": " << match.text(doc) << "\n";
  }

  std::cout << "\nPhrases\n";
  for (const auto& phrase : doc.phrase_texts()) {
    std::cout << "  " << phrase << "\n";
  }

  const auto tmp = std::filesystem::temp_directory_path() / "nodeb_full_nlp_workflow_demo.bin";
  nlp.save(tmp.string());
  const auto restored = Workflow::load(tmp.string());
  std::filesystem::remove(tmp);
  const auto restored_doc = restored.infer(probe);

  std::cout << "\nRoundtrip\n";
  std::cout << "  restored_label=" << restored_doc.doc_label
            << " restored_score=" << restored_doc.doc_score
            << " restored_entities=" << restored_doc.entities.size()
            << " restored_matches=" << restored_doc.matches.size()
            << "\n";

  return 0;
}
