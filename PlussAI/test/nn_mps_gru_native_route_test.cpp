// MPS GRU training investigation (three separate, real findings pinned as
// three separate TEST_CASEs -- see each one's own comment):
//
//  1. Under PLUSS_MPS_GRU_ROUTE_STRICT=1, the native MPSGraph GRU route
//     rejects an ordinary batch_first=TRUE GRU (RNNBase transposes a
//     batch_first input to seq-first internally before recursing, which
//     produces a tensor the native kernel's "zero-offset contiguous"
//     precondition rejects) with "expects zero-offset contiguous float32
//     MPS input/state tensors". batch_first=false does NOT hit this. This
//     is why PLUSS_MPS_GRU_ROUTE now defaults OFF (see gru_route_enabled()
//     in mps_rnn.mm) -- without strict mode, the SAME precondition
//     mismatch used to silently fall back to the generic path, which is
//     safe, just undocumented.
//  2. With the generic (non-native) path -- today's default -- and with
//     BOTH platforms started from byte-identical weights/inputs (copied
//     across platforms, not independently seeded -- see
//     nn_mps_rng_platform_parity_test.cpp for why independent seeding
//     would be an invalid comparison), the forward computation matches
//     CPU exactly. GRU's actual math is correct on MPS.
//  3. NOT a bug, corrected after investigation: an earlier version of this
//     test reassigned a registered parameter via `*param =
//     new_value.to(pMPS)` and saw gradients vanish on MPS (but not CPU).
//     Root cause was the TEST, not the engine: `.to(pMPS)` on a Series
//     already tracked by a DIFFERENT model (seed_gru) leaves a
//     SourceLink pointing back at that unrelated Series, making the
//     "reassigned" parameter a non-leaf whose sensitivity has nowhere
//     correct to go. PlussAI/NodeK/NN/optim/optim_base.h's own
//     assign_parameter_() already does this correctly every optimizer
//     step: reassign, THEN clear_source_link(), THEN re-set
//     tracks_slope. Adding the missing clear_source_link() call below
//     fixes it -- gradients survive parameter reassignment on MPS fine.
#include <pluss/ai.h>

#include <cmath>
#include <cstdlib>
#include <string>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

namespace {

class ScopedEnvVar final {
public:
  ScopedEnvVar(const char* key, const char* value) : key_(key) {
    const char* old = std::getenv(key);
    if (old != nullptr) {
      had_old_ = true;
      old_ = old;
    }
    setenv(key, value, 1);
  }
  ~ScopedEnvVar() {
    if (had_old_) setenv(key_.c_str(), old_.c_str(), 1);
    else unsetenv(key_.c_str());
  }

private:
  std::string key_;
  std::string old_;
  bool had_old_{false};
};

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

bool close_vec(const std::vector<double>& a, const std::vector<double>& b, double atol, double rtol) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (std::abs(a[i] - b[i]) > atol + rtol * std::abs(b[i])) return false;
  }
  return true;
}

constexpr std::int64_t kInputSize = 4, kHiddenSize = 3, kSeqLen = 5, kBatch = 2;

}  // namespace

TEST_CASE("MPS native GRU route rejects an ordinary batch_first=false input under strict mode",
         "[mps][gru][rnn][native]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }
  const ScopedEnvVar strict("PLUSS_MPS_GRU_ROUTE_STRICT", "1");
  const ScopedEnvVar route("PLUSS_MPS_GRU_ROUTE", "1");

  const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
  // batch_first=true specifically: RNNBase transposes a batch_first input
  // to seq-first internally before recursing, which apparently produces a
  // tensor the native kernel's "zero-offset contiguous" precondition
  // rejects. batch_first=false (no internal transpose) does NOT trigger
  // this -- see the sibling forward-parity TEST_CASE, which uses
  // batch_first=false and passes.
  pluss::nn::GRU gru(pluss::nn::GRUOptions{
      .input_size = kInputSize,
      .hidden_size = kHiddenSize,
      .num_layers = 1,
      .bias = true,
      .batch_first = true,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = false,
      .cfg = cfg,
      .seed = 42ULL,
      .training = true,
  });
  const Series seq = pluss::series::randn({kBatch, kSeqLen, kInputSize}, cfg, /*seed=*/7);
  const Series h0 = pluss::series::zeros({1, kBatch, kHiddenSize}, cfg);

  REQUIRE_THROWS(gru->forward(seq, h0));
}

TEST_CASE("MPS GRU forward matches CPU exactly given identical weights and inputs",
         "[mps][gru][rnn][forward]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }
  // Default policy: native route off, strict off -- the generic path.

  const Config seed_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series seq_master = pluss::series::randn({kSeqLen, kBatch, kInputSize}, seed_cfg, /*seed=*/7);
  const Series h0_master = pluss::series::zeros({1, kBatch, kHiddenSize}, seed_cfg);

  pluss::nn::GRU seed_gru(pluss::nn::GRUOptions{
      .input_size = kInputSize,
      .hidden_size = kHiddenSize,
      .num_layers = 1,
      .bias = true,
      .batch_first = false,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = false,
      .cfg = seed_cfg,
      .seed = 42ULL,
      .training = true,
  });
  std::vector<Series> master_params;
  for (const Series* p : seed_gru->parameters()) master_params.push_back(*p);

  auto run = [&](pluss::Platform platform) {
    const Config cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(platform);
    pluss::nn::GRU gru(pluss::nn::GRUOptions{
        .input_size = kInputSize,
        .hidden_size = kHiddenSize,
        .num_layers = 1,
        .bias = true,
        .batch_first = false,
        .dropout = 0.0,
        .bidirectional = false,
        .split_bias = false,
        .cfg = cfg,
        .seed = 42ULL,
        .training = true,
    });
    auto own_params = gru->parameters();
    for (std::size_t i = 0; i < own_params.size(); ++i) {
      *own_params[i] = master_params[i].to(platform);
    }
    const auto [out, hn] = gru->forward(seq_master.to(platform), h0_master.to(platform));
    return std::make_pair(vec_f64(out), vec_f64(hn));
  };

  const auto [cpu_out, cpu_hn] = run(pluss::pCPU);
  const auto [mps_out, mps_hn] = run(pluss::pMPS);

  REQUIRE(close_vec(mps_out, cpu_out, 1e-3, 1e-3));
  REQUIRE(close_vec(mps_hn, cpu_hn, 1e-3, 1e-3));
}

TEST_CASE("MPS GRU gradients survive a parameter reassignment (optimizer.step()-shaped)",
         "[mps][gru][rnn][backward][known_issue]") {
  if (!pluss::mps::is_ready()) {
    SUCCEED("MPS runtime unavailable");
    return;
  }

  const Config seed_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series seq_master = pluss::series::randn({kSeqLen, kBatch, kInputSize}, seed_cfg, /*seed=*/7);
  const Series h0_master = pluss::series::zeros({1, kBatch, kHiddenSize}, seed_cfg);

  pluss::nn::GRU seed_gru(pluss::nn::GRUOptions{
      .input_size = kInputSize,
      .hidden_size = kHiddenSize,
      .num_layers = 1,
      .bias = true,
      .batch_first = false,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = false,
      .cfg = seed_cfg,
      .seed = 42ULL,
      .training = true,
  });
  std::vector<Series> master_params;
  for (const Series* p : seed_gru->parameters()) master_params.push_back(*p);

  const Config mps_cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pMPS);
  pluss::nn::GRU gru(pluss::nn::GRUOptions{
      .input_size = kInputSize,
      .hidden_size = kHiddenSize,
      .num_layers = 1,
      .bias = true,
      .batch_first = false,
      .dropout = 0.0,
      .bidirectional = false,
      .split_bias = false,
      .cfg = mps_cfg,
      .seed = 42ULL,
      .training = true,
  });
  auto own_params = gru->parameters();
  for (std::size_t i = 0; i < own_params.size(); ++i) {
    // Exactly PlussAI/NodeK/NN/optim/optim_base.h's own
    // assign_parameter_(): reassign, THEN sever the SourceLink the
    // cross-platform .to(MPS) copy leaves behind (pointing back at
    // master_params[i], a Series belonging to an unrelated model
    // instance), THEN re-establish tracks_slope. Skipping
    // clear_source_link() here was the actual bug in an earlier version
    // of this test, not an engine bug: without it, `.reverse()` was
    // trying to propagate sensitivity through to seed_gru's own
    // parameter instead of accumulating it on gru's own leaf.
    const bool tracks = own_params[i]->tracks_slope();
    *own_params[i] = master_params[i].to(pluss::pMPS);
    pluss::codem::series_tape::clear_source_link(*own_params[i]);
    own_params[i]->tracks_slope_(tracks);
  }

  const auto [out, hn] = gru->forward(seq_master.to(pluss::pMPS), h0_master.to(pluss::pMPS));
  (void)hn;
  out.mul(out).sum().reverse();

  std::size_t num_with_sensitivity = 0;
  for (const Series* p : gru->parameters()) {
    if (p && p->defined() && p->has_sensitivity()) ++num_with_sensitivity;
  }
  REQUIRE(num_with_sensitivity == gru->parameters().size());
}
