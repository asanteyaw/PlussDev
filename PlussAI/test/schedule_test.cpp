#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <cmath>
#include <iostream>
#include <tuple>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

bool approx_vec(const std::vector<double>& a, const std::vector<double>& b, double tol = 1e-9) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i] - b[i]) > tol) return false;
  }
  return true;
}

bool throws(const auto& fn) {
  try {
    fn();
  } catch (...) {
    return true;
  }
  return false;
}

bool run_schedule_checks(std::ostream* err = nullptr) {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  pluss::nn::Schedule empty_schedule;
  if (!throws([&] {
        const Series x = pluss::series::zeros({1, 2}, cfg);
        (void)empty_schedule->forward(x);
      })) {
    if (err) *err << "empty schedule should fail loudly\n";
    return false;
  }

  pluss::nn::Schedule model;
  model.then("fc1", pluss::nn::Fusion(2, 2, true, cfg, 11ULL))
      .then("act", pluss::nn::ReLU())
      .then("head", pluss::nn::Fusion(2, 1, true, cfg, 13ULL));

  if (model->size() != 3 || model->empty()) {
    if (err) *err << "size/empty mismatch\n";
    return false;
  }
  if (!model->contains("act") || model->index_of("head") != 2) {
    if (err) *err << "contains/index_of mismatch\n";
    return false;
  }
  if (model->name_at(0) != "fc1" || model->name_at(1) != "act" || model->name_at(2) != "head") {
    if (err) *err << "name_at mismatch\n";
    return false;
  }

  if (!throws([&] {
        model.then("act", pluss::nn::Identity());
      })) {
    if (err) *err << "duplicate name should fail loudly\n";
    return false;
  }

  model.insert(2, "bridge", pluss::nn::Identity());
  if (model->size() != 4 || model->index_of("bridge") != 2) {
    if (err) *err << "insert mismatch\n";
    return false;
  }
  model->erase(2);
  if (model->size() != 3 || model->contains("bridge")) {
    if (err) *err << "erase mismatch\n";
    return false;
  }

  const Series x = pluss::make_series(std::vector<double>{1.0, -1.0, 0.2, 0.7}, {2, 2}, cfg);
  const Series y0 = model->forward(x);
  const Series y1 = model.run<Series>(x);
  if (!approx_vec(y0.to_vec<double>(), y1.to_vec<double>())) {
    if (err) *err << "forward/run mismatch\n";
    return false;
  }

  const auto manifest = model->stage_manifest();
  if (manifest.size() != 3 || manifest[0].name != "fc1" || manifest[1].name != "act" || manifest[2].name != "head") {
    if (err) *err << "stage_manifest mismatch\n";
    return false;
  }
  if (!manifest[0].module_stage || !manifest[1].module_stage || !manifest[2].module_stage) {
    if (err) *err << "module_stage flags mismatch\n";
    return false;
  }
  if (model->module_at(1) == nullptr) {
    if (err) *err << "module_at should return valid module pointer\n";
    return false;
  }

  pluss::nn::Schedule tail{pluss::nn::Identity()};
  model.extend(tail);
  if (model->size() != 4) {
    if (err) *err << "extend(schedule) size mismatch\n";
    return false;
  }

  pluss::nn::Schedule typed;
  typed.then_as<std::tuple<int, int>, int>("sum", [](std::tuple<int, int> pair) {
    return std::get<0>(pair) + std::get<1>(pair);
  });
  typed.then_as<int, int>("double", [](int v) {
    return 2 * v;
  });
  if (typed.run<int>(2, 3) != 10) {
    if (err) *err << "typed run mismatch\n";
    return false;
  }
  if (!throws([&] {
        (void)typed.run<int>(5);
      })) {
    if (err) *err << "typed first-input mismatch should fail loudly\n";
    return false;
  }
  if (!throws([&] {
        typed.insert_as<Series, Series>(1, "bad", pluss::nn::Identity());
      })) {
    if (err) *err << "typed chain compatibility should fail loudly\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("schedule surface stays aligned") {
  REQUIRE(run_schedule_checks(&std::cerr));
}
#else
int main() {
  if (!run_schedule_checks(&std::cerr)) return 1;
  std::cout << "schedule_test passed\n";
  return 0;
}
#endif
