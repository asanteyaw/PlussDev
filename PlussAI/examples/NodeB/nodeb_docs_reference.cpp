#include <pluss/nlp.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow pipeline{
      .lang = nlp::Language::English,
      .stages = {
          nlp::stage::tokenize(),
          nlp::stage::sentencize(),
          nlp::stage::lemmatize(),
          nlp::stage::matcher({
              nlp::MatcherPattern{
                  .label = "TOPIC",
                  .tokens = {"write", "api"},
                  .use_lemmas = true,
              },
          }),
      },
  };

  const auto doc = pipeline.infer("Ama writes APIs. Kojo tests them!");
  std::cout << "scalar\n";
  std::cout << "tokens=" << doc.tokens.size()
            << " sentences=" << doc.sentences.size()
            << " matches=" << doc.matches.size() << '\n';
  std::cout << "lemmas=";
  const auto lemmas = doc.lemma_texts();
  for (std::size_t i = 0; i < lemmas.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << lemmas[i];
  }
  std::cout << "\nmatch=" << doc.matches.front().label << ':'
            << doc.matches.front().text(doc) << '\n';

  const auto batch = pipeline.infer(
      pluss::make_note({"One sentence.", "Two sentences. Really!"}));
  const auto report = pipeline.assess(batch,
      nlp::AssessOptions{.require_sentences = true});
  std::cout << "batch\n";
  std::cout << "docs=" << report.doc_count
            << " tokens=" << report.token_count
            << " sentences=" << report.sentence_count
            << " mean_tokens=" << report.mean_tokens_per_doc << '\n';

  const auto path = std::filesystem::temp_directory_path() /
                    "pluss_nlp_docs_reference.workflow";
  pipeline.save(path.string());
  const auto restored = nlp::Workflow::load(path.string());
  std::filesystem::remove(path);
  const auto roundtrip = restored.infer("Ama writes APIs.");
  std::cout << "roundtrip\n";
  std::cout << "language=" << nlp::resources::language_code(restored.lang)
            << " stages=" << restored.stages.size()
            << " tokens=" << roundtrip.tokens.size()
            << " matches=" << roundtrip.matches.size() << '\n';
}
