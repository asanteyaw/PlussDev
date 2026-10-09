#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64);
  const pluss::table::Slate cat = pluss::table::Slate(pluss::make_note({"red", "blue", "red", "blue", "red", "blue"}));
  const pluss::Series y = x.mul(2.0).add(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0, 0.0, 1.0}, {6}, f64));

  const pluss::table::DataFrame housing({
      {"median_income", pluss::table::Slate(x)},
      {"ocean_proximity", cat},
      {"target", pluss::table::Slate(y)},
  });

  const auto numeric = housing.feataxis().series().features();
  const auto categorical = housing.feataxis().note().features();

  lymph::DataFrameBatchOptions data_options{};
  data_options.feature_names = {"median_income", "ocean_proximity"};
  data_options.label_feature = "target";
  data_options.note_policy = lymph::DataNotePolicy::Hash;
  data_options.layout = lymph::ObservationLayout::Rows;

  lymph::prep::FeatureTransformer preprocessing = lymph::prep::feature_transformer(
      housing,
      data_options,
      std::vector<lymph::prep::branch::Branch>{
          lymph::prep::branch::compose(
              {
                  lymph::prep::stage::simple_imputer(lymph::prep::SimpleImputerOptions{
                      .strategy = lymph::prep::SimpleImputerStrategy::Mean,
                  }),
                  lymph::prep::stage::standard_scaler(),
              },
              {"median_income"}),
          lymph::prep::branch::one_hot_encoder(lymph::prep::OneHotEncoderOptions{}, categorical),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  auto reg = lymph::models::linear_regression(lymph::LinearFamilyOptions{
      .objective = lymph::LinearObjective::LeastSquares,
      .regularization = lymph::RegularizationKind::None,
      .regularization_strength = 0.0,
      .fit_intercept = true,
  });
  reg.feature_transformer(std::move(preprocessing));
  reg.learn_(housing, data_options);

  const auto pred = reg.infer(housing, data_options);
  const auto metrics = reg.assess(housing,
                                  data_options,
                                  lymph::AssessOptions{
                                      .metrics = {lymph::MetricKind::Mse, lymph::MetricKind::R2},
                                  });

  std::cout << "numeric features: ";
  for (const auto& n : numeric) std::cout << n << " ";
  std::cout << "\n";
  std::cout << "categorical features: ";
  for (const auto& n : categorical) std::cout << n << " ";
  std::cout << "\n";
  auto ord = lymph::prep::OrdinalEncoder({.include_unknown_bucket = true, .one_based = true});
  ord.features({"ocean_proximity"});
  const auto ocean_ord = ord.learn_map(housing.features({"ocean_proximity"}));
  auto lbl = lymph::prep::LabelEncoder({.include_unknown_bucket = true, .one_based = false});
  const auto ocean_lbl = lbl.learn_map(housing.features("ocean_proximity"));
  std::cout << "ordinal mapped names: ";
  for (const auto& name : ord.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "label mapped names: ";
  for (const auto& name : lbl.mapped_names_) std::cout << name << " ";
  std::cout << "\n";
  std::cout << "ordinal sample: " << ocean_ord << "\n";
  std::cout << "label sample: " << ocean_lbl << "\n";
  std::cout << "pred: " << pred << "\n";
  std::cout << "mse: " << metrics.mse << " r2: " << metrics.r2 << "\n";
  return 0;
}
