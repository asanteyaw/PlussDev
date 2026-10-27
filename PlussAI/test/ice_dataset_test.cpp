#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
namespace data = pluss::ai::ice::data;

Config float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

DataFrame make_cross_section_train() {
  return DataFrame({
      {"city", Slate(pluss::make_note({"Accra", "Tema", "Accra", "Tema"}))},
      {"sales", Slate(pluss::make_series({10.0, 7.0, 13.0, 9.0}, float_cfg()))},
      {"label", Slate(pluss::make_series({1.0, 0.0, 1.0, 0.0}, float_cfg()))},
  });
}

DataFrame make_cross_section_holdout() {
  return DataFrame({
      {"city", Slate(pluss::make_note({"Kumasi", "Accra"}))},
      {"sales", Slate(pluss::make_series({11.0, 8.0}, float_cfg()))},
      {"label", Slate(pluss::make_series({0.0, 1.0}, float_cfg()))},
  });
}

bool run_cross_section_dataset_checks(std::ostream* err = nullptr) {
  data::DataSpec spec{};
  spec.layout = data::Layout::CrossSection;
  spec.target = "label";
  spec.fields = {
      {"city", data::Role::FeatureCategorical},
      {"sales", data::Role::FeatureContinuous},
  };
  spec.cfg = float_cfg();

  const auto train = data::DataSet::from_frame(make_cross_section_train(), spec);
  if (train.size() != 4) {
    if (err) *err << "CrossSection DataSet size mismatch\n";
    return false;
  }
  if (!train.batch().has(data::field_names::features)) {
    if (err) *err << "CrossSection DataSet should materialize a 'features' field\n";
    return false;
  }
  const Series features = train.batch().field(data::field_names::features);
  const auto sizes = features.sizes();
  if (sizes.size() != 2 || sizes[0] != 4 || sizes[1] != 2) {
    if (err) *err << "CrossSection features shape should be [N, F]\n";
    return false;
  }
  if (train.batch().categoricals(data::field_names::features).size() != 1 ||
      train.batch().categoricals(data::field_names::features)[0].variable_index != 0 ||
      train.batch().categoricals(data::field_names::features)[0].cardinality != 3) {
    if (err) *err << "CrossSection categorical bookkeeping should preserve slot and unknown bucket\n";
    return false;
  }

  const auto holdout = data::DataSet::from_frame(make_cross_section_holdout(), spec, train);
  const auto encoded = holdout.batch()
                           .field(data::field_names::features)
                           .narrow_copy(1, 0, 1)
                           .to(pluss::pCPU, ScalarType::Float64)
                           .reshape({holdout.size()})
                           .host<double>();
  if (std::abs(encoded[0] - 2.0) > 1e-8) {
    if (err) *err << "Unseen categorical value should map to reserved unknown code in reused vocabularies\n";
    return false;
  }
  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("ice generic dataset supports cross-sectional feature batches and vocabulary reuse",
          "[ai][ice][data]") {
  REQUIRE(run_cross_section_dataset_checks());
}
#else
int main() {
  if (!run_cross_section_dataset_checks(&std::cerr)) return 1;
  std::cout << "ice_dataset_test passed\n";
  return 0;
}
#endif
