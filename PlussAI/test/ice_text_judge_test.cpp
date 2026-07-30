#include <pluss/ai/ice/models/nlp/text_judge.h>
#include <pluss/pluss.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::ai::ice::models::TextJudge;
using pluss::ai::ice::models::TextJudgeOptions;
using pluss::ai::ice::models::TextJudgeTrainOptions;

[[nodiscard]] std::vector<std::string> corpus() {
  std::vector<std::string> texts{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("stocks rallied after the earnings report");
    texts.push_back("the market closed higher on strong profits");
    texts.push_back("bonds sold off as yields climbed");
    texts.push_back("the striker scored twice in the cup match");
    texts.push_back("the goalkeeper saved a late penalty");
    texts.push_back("fans celebrated the league victory downtown");
  }
  return texts;
}

[[nodiscard]] std::vector<std::string> labels() {
  std::vector<std::string> out{};
  out.reserve(24 * 6);
  for (int i = 0; i < 24; ++i) {
    out.push_back("finance");
    out.push_back("finance");
    out.push_back("finance");
    out.push_back("sport");
    out.push_back("sport");
    out.push_back("sport");
  }
  return out;
}

bool run_ice_text_judge_checks(std::ostream* err = nullptr) {
  TextJudgeOptions model_options{};
  model_options.width = 24;
  model_options.dropout = 0.0;
  model_options.seed = 9;

  TextJudge model(model_options);
  if (model.ready()) {
    if (err) *err << "ice TextJudge should not report ready before learn()\n";
    return false;
  }

  TextJudgeTrainOptions train_options{};
  train_options.batch_size = 12;
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 120;
  train_options.validation.split_ratio = 0.15;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 12;
  train_options.loop_regularization.restore_best_weights = true;
  train_options.seed = 13;

  const auto train_texts = corpus();
  const auto train_labels = labels();
  model.learn(train_texts, train_labels, train_options);

  if (!model.ready()) {
    if (err) *err << "ice TextJudge should report ready after learn()\n";
    return false;
  }
  if (model.labels().size() != 2) {
    if (err) *err << "ice TextJudge should learn exactly two label values\n";
    return false;
  }

  const auto report = model.assess(train_texts, train_labels);
  if (report.accuracy < 0.95) {
    if (err) {
      *err << "ice TextJudge should fit the clean corpus; got accuracy=" << report.accuracy
           << " loss=" << report.loss << "\n";
      const auto debug_preds = model.predict(std::vector<std::string>{
          "stocks rallied after the earnings report",
          "the striker scored twice in the cup match",
          "the market closed higher on strong profits",
          "fans celebrated the league victory downtown",
      });
      for (std::size_t i = 0; i < debug_preds.size(); ++i) {
        *err << "debug_pred[" << i << "]=" << debug_preds[i].label
             << " conf=" << debug_preds[i].confidence << "\n";
      }
    }
    return false;
  }

  const std::vector<std::string> probe_texts{
      "profits lifted the stock market today",
      "the forward won the match with a late goal",
  };
  const auto preds = model.predict(probe_texts);
  if (preds.size() != 2 ||
      preds[0].label != "finance" ||
      preds[1].label != "sport" ||
      preds[0].confidence <= 0.5 ||
      preds[1].confidence <= 0.5) {
    if (err) {
      *err << "ice TextJudge should separate finance vs sport probes; got "
           << preds[0].label << " (" << preds[0].confidence << "), "
           << preds[1].label << " (" << preds[1].confidence << ")\n";
    }
    return false;
  }

  const auto tmp = std::filesystem::temp_directory_path() / "ice_text_judge_roundtrip.bin";
  model.save(tmp.string());
  const auto restored = TextJudge::load(tmp.string());
  std::filesystem::remove(tmp);
  std::filesystem::remove(tmp.string() + ".weights");

  const auto restored_preds = restored.predict(probe_texts);
  if (restored_preds.size() != preds.size()) {
    if (err) *err << "ice TextJudge save/load should preserve prediction count\n";
    return false;
  }
  for (std::size_t i = 0; i < preds.size(); ++i) {
    if (restored_preds[i].label != preds[i].label ||
        std::abs(restored_preds[i].confidence - preds[i].confidence) > 1e-9) {
      if (err) *err << "ice TextJudge save/load should preserve predictions exactly\n";
      return false;
    }
  }

  return true;
}

bool run_ice_text_judge_punctuation_checks(std::ostream* err = nullptr) {
  TextJudgeOptions model_options{};
  model_options.width = 16;
  model_options.use_lemmas = false;
  model_options.skip_punct = false;
  model_options.lowercase = true;
  model_options.dropout = 0.0;
  model_options.seed = 21;

  TextJudgeTrainOptions train_options{};
  train_options.learning_rate = 5e-2;
  train_options.execution.max_epochs = 40;
  train_options.validation.split_ratio = 0.25;
  train_options.validation.monitor = "val_accuracy";
  train_options.validation.every_n_epochs = 2;
  train_options.validation.start_from_epoch = 1;
  train_options.loop_regularization.early_stopping = true;
  train_options.loop_regularization.patience = 4;
  train_options.loop_regularization.restore_best_weights = true;
  train_options.seed = 33;

  std::vector<std::string> texts{};
  std::vector<std::string> labels{};
  for (int i = 0; i < 24; ++i) {
    texts.push_back("alpha !");
    labels.push_back("excited");
    texts.push_back("alpha ?");
    labels.push_back("curious");
  }

  TextJudge model(model_options);
  model.learn(texts, labels, train_options);

  const auto preds = model.predict(std::vector<std::string>{"alpha !", "alpha ?"});
  if (preds.size() != 2 ||
      preds[0].label != "excited" ||
      preds[1].label != "curious") {
    if (err) {
      *err << "ice TextJudge should honor skip_punct=false; got "
           << preds[0].label << " and " << preds[1].label << "\n";
    }
    return false;
  }

  const auto tmp =
      std::filesystem::temp_directory_path() / "ice_text_judge_punct_roundtrip.bin";
  model.save(tmp.string());
  const auto restored = TextJudge::load(tmp.string());
  std::filesystem::remove(tmp);

  const auto restored_preds = restored.predict(std::vector<std::string>{"alpha !", "alpha ?"});
  if (restored_preds.size() != 2 ||
      restored_preds[0].label != "excited" ||
      restored_preds[1].label != "curious") {
    if (err) {
      *err << "ice TextJudge save/load should preserve punctuation-sensitive behavior; got "
           << restored_preds[0].label << " and " << restored_preds[1].label << "\n";
    }
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice TextJudge trains and round-trips", "[ai][ice][nlp]") {
  if (!run_ice_text_judge_checks(&std::cerr)) {
    FAIL("ice TextJudge checks failed");
  }
}

TEST_CASE("ice TextJudge preserves punctuation-sensitive sequence settings", "[ai][ice][nlp]") {
  if (!run_ice_text_judge_punctuation_checks(&std::cerr)) {
    FAIL("ice TextJudge punctuation checks failed");
  }
}
#else
int main() {
  if (!run_ice_text_judge_checks(&std::cerr)) {
    return 1;
  }
  if (!run_ice_text_judge_punctuation_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "ice TextJudge checks passed\n";
  return 0;
}
#endif
