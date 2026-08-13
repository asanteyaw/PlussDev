#include <pluss/ai/procedure.h>
#include <pluss/nn.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  namespace ice = pluss::ai::ice;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<float>{-2.0F, 0.0F, 3.0F}, {3, 1}, cfg);
  pluss::nn::ReLU relu{};

  const ai::Procedure nodek_proc = ai::procedure::make({
      ai::procedure::step("relu", "x", "positive", relu),
  });
  const ai::ProcedureResult nodek_out = nodek_proc.run({ai::procedure::input("x", x)});

  auto model = ice::Outline(pluss::nn::Fusion(1, 1));
  ice::TrainOptions train{};
  train.execution.max_epochs = 1;
  train.validation.split_ratio = 0.0;
  train.batch_size = 3;
  train.learning_rate = 0.01;

  const pluss::Series target = pluss::make_series(std::vector<float>{0.0F, 0.0F, 3.0F}, {3, 1}, cfg);
  ice::Batch batch{};
  batch.set_inputs(x);
  batch.set_targets(target);
  ai::RunOptions learn{};
  learn.mode = ai::ProcedureMode::Learn;

  const ai::Procedure ice_proc = ai::procedure::make({
      ai::procedure::step("learn", "train", "done", static_cast<ice::Model&>(model), ice::LossKind::MSE, train),
  });
  const ai::ProcedureResult ice_out = ice_proc.run({ai::procedure::input("train", batch)}, learn);

  std::cout << "nodek_positive=" << nodek_out.get<pluss::Series>("positive").numel() << "\n";
  std::cout << "ice_trained=" << ice_out.get<bool>("done") << "\n";
  return 0;
}
