#include <pluss/ai/procedure.h>
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace nodeb = pluss::ai::nodeb;

  nodeb::Workflow nlp{
      .lang = nodeb::Language::English,
      .stages = {
          nodeb::stage::tokenize(),
          nodeb::stage::sentencize(),
          nodeb::stage::lemmatize(),
      },
  };
  const std::vector<std::string> texts = {
      "Procedure runs NLP as one deployable step.",
      "The graph can pass documents into later AI work.",
  };

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("nlp", "text", "docs", nlp),
      ai::procedure::step("store", ai::procedure::sink("procedure.demo.docs")),
  }, {
      ai::procedure::link("nlp", "store"),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("text", texts)});
  std::cout << "docs=" << out.get<std::vector<nodeb::Doc>>("docs").size() << "\n";
  std::cout << "stored=" << out.get<std::vector<nodeb::Doc>>("store").size() << "\n";
  return 0;
}
