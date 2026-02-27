#include "../dfo_minimizers.h"
#include "../mads_blackbox_minimizers.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/wait.h>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

namespace fs = std::filesystem;

struct CommandResult {
  int exit_code{1};
  std::string output{};
};

double as_f64_scalar(const Series& s) {
  if (!s.defined() || s.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return s.to(pCPU, ScalarType::Float64).item<double>();
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

bool require_strict_external_refs() {
  const char* env = std::getenv("PLUSS_REQUIRE_EXTERNAL_PARITY");
  return env != nullptr && std::string(env) == "1";
}

std::string shell_quote(const std::string& raw) {
  std::string out;
  out.reserve(raw.size() + 8);
  out.push_back('\'');
  for (char c : raw) {
    if (c == '\'') {
      out += "'\"'\"'";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

CommandResult run_command_capture(const std::string& cmd) {
  CommandResult r;
  std::array<char, 4096> buffer{};

  FILE* pipe = popen(cmd.c_str(), "r");
  if (pipe == nullptr) {
    throw std::runtime_error("popen failed for command: " + cmd);
  }

  while (true) {
    const std::size_t n = std::fread(buffer.data(), 1, buffer.size(), pipe);
    if (n > 0) {
      r.output.append(buffer.data(), n);
    }
    if (n < buffer.size()) {
      if (std::feof(pipe)) {
        break;
      }
      if (std::ferror(pipe)) {
        break;
      }
    }
  }

  const int status = pclose(pipe);
#if defined(__unix__) || defined(__APPLE__)
  if (WIFEXITED(status)) {
    r.exit_code = WEXITSTATUS(status);
  } else {
    r.exit_code = status;
  }
#else
  r.exit_code = status;
#endif
  return r;
}

fs::path find_repo_root() {
  fs::path p = fs::absolute(fs::path(__FILE__));
  if (p.has_filename()) {
    p = p.parent_path();
  }
  while (!p.empty()) {
    if (fs::exists(p / "PlussMa") && fs::exists(p / "PlussQit")) {
      return p;
    }
    const fs::path parent = p.parent_path();
    if (parent == p) {
      break;
    }
    p = parent;
  }
  return {};
}

std::optional<double> parse_nomad_best_feasible_obj(const std::string& text) {
  std::regex re(
      R"(Best feasible solution:.*f\s*=\s*([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?))");
  std::smatch m;
  std::string::const_iterator begin = text.begin();
  std::string::const_iterator end = text.end();
  while (std::regex_search(begin, end, m, re)) {
    if (m.size() >= 2) {
      return std::stod(m[1].str());
    }
    begin = m.suffix().first;
  }
  return std::nullopt;
}

std::tuple<double, double, double, double> nomad_example1_eval(const std::vector<double>& x) {
  const std::size_t n = x.size();
  double sum1 = 0.0;
  double sum2 = 0.0;
  double sum3 = 0.0;
  double prod1 = 1.0;
  double prod2 = 1.0;

  for (std::size_t i = 0; i < n; ++i) {
    const double xi = x[i];
    const double c = std::cos(xi);
    sum1 += std::pow(c, 4.0);
    sum2 += xi;
    sum3 += static_cast<double>(i + 1) * xi * xi;
    prod1 *= c * c;
    if (prod2 != 0.0) {
      if (xi == 0.0) {
        prod2 = 0.0;
      } else {
        prod2 *= xi;
      }
    }
  }

  const double g1 = -prod2 + 0.75;
  const double g2 = sum2 - 7.5 * static_cast<double>(n);

  double f = 10.0 * g1 + 10.0 * g2;
  if (sum3 != 0.0) {
    f -= std::abs((sum1 - 2.0 * prod1) / std::abs(std::sqrt(sum3)));
  }
  if (!std::isnan(f)) {
    f *= 1e-5;
  }
  const double g3 = -(f + 2000.0);
  return {f, g1, g2, g3};
}

std::vector<double> to_host_f64_vec(const Series& x) {
  const Series cpu = x.to(pCPU, ScalarType::Float64).contiguous();
  return cpu.to_vec<double>();
}

} // namespace

int main() {
  int failures = 0;
  const bool strict_external = require_strict_external_refs();

  const fs::path repo = find_repo_root();
  if (repo.empty()) {
    std::cerr << "[FAIL] Could not resolve repository root from __FILE__\n";
    return 1;
  }

  // ---- OptimMaster external parity readiness gate ----
  const fs::path optim_bmo =
      repo / "optim-master" / "include" / "BaseMatrixOps" / "include" / "BaseMatrixOps.hpp";
  if (!fs::exists(optim_bmo)) {
    const std::string msg =
        "optim-master external parity is unavailable (missing include/BaseMatrixOps submodule content)";
    if (strict_external) {
      std::cerr << "[FAIL] " << msg << '\n';
      ++failures;
    } else {
      std::cerr << "[SKIP] " << msg << '\n';
    }
  }

  // ---- NOMAD side-by-side parity ----
  fs::path nomad_bin = repo / "nomad-master" / "build" / "release" / "bin" / "nomad";
  if (!fs::exists(nomad_bin)) {
    nomad_bin = repo / "nomad-master" / "build" / "release" / "src" / "nomad";
  }
  const fs::path bb_exe = repo / "nomad-master" / "build" / "release" / "examples" / "basic" / "batch" /
                          "example1" / "bb1.exe";

  if (!fs::exists(nomad_bin) || !fs::exists(bb_exe)) {
    const std::string msg =
        "NOMAD external parity is unavailable (expected built binaries under nomad-master/build/release)";
    if (strict_external) {
      std::cerr << "[FAIL] " << msg << '\n';
      ++failures;
    } else {
      std::cerr << "[SKIP] " << msg << '\n';
    }
  } else {
    const std::int64_t n_dim = 10;
    const std::int64_t max_bb_eval = 1000;
    const std::uint64_t seed = 20260226ULL;

    const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
    const Series x0 = pluss::full({n_dim}, 5.0, cfg);
    const Series lb = pluss::full({n_dim}, -20.0, cfg);
    const Series ub = pluss::full({n_dim}, 20.0, cfg);

    const fs::path run_dir = fs::temp_directory_path() / "pluss_nomad_external_parity";
    fs::create_directories(run_dir);
    const fs::path param_file = run_dir / "param_parity.txt";

    {
      std::ofstream out(param_file);
      out << "DIMENSION 10\n";
      out << "BB_EXE " << bb_exe.string() << "\n";
      out << "BB_OUTPUT_TYPE OBJ PB PB EB\n";
      out << "X0 ( 5 5 5 5 5 5 5 5 5 5 )\n";
      out << "LOWER_BOUND * -20\n";
      out << "UPPER_BOUND * 20\n";
      out << "GRANULARITY ( 1 1 0.5 1 1 1 1 0.5 1 1 )\n";
      out << "MAX_BB_EVAL " << max_bb_eval << "\n";
      out << "MAX_EVAL 1000000\n";
      out << "SEED " << seed << "\n";
      out << "DISPLAY_STATS BBE ( SOL ) OBJ\n";
    }

    const std::string nomad_cmd =
        shell_quote(nomad_bin.string()) + " " + shell_quote(param_file.string()) + " 2>&1";
    const CommandResult nomad_run = run_command_capture(nomad_cmd);
    expect(nomad_run.exit_code == 0, "NOMAD run exited with non-zero status", &failures);

    const auto nomad_obj = parse_nomad_best_feasible_obj(nomad_run.output);
    expect(nomad_obj.has_value(), "Could not parse NOMAD best feasible objective", &failures);

    if (nomad_obj.has_value()) {
      MadsBlackboxAdvancedConfig mads_cfg;
      mads_cfg.max_iterations = 3000;
      mads_cfg.max_evaluations = max_bb_eval;
      mads_cfg.max_total_evaluations = 200000;
      mads_cfg.check_every = 10;
      mads_cfg.search_points = 0;
      mads_cfg.relative_improvement_tolerance = 1e-12;
      mads_cfg.mesh_tolerance = 1e-9;
      mads_cfg.lower_bounds = lb;
      mads_cfg.upper_bounds = ub;
      mads_cfg.opportunistic = true;
      mads_cfg.poll_scheme = MadsPollScheme::Ortho2N;
      mads_cfg.rng_seed = seed;
      mads_cfg.display = BlackboxSolverDisplay::None;
      mads_cfg.constraint_handling = ConstraintHandling::ProgressiveBarrier;
      mads_cfg.constraint_tolerance = 1e-12;
      mads_cfg.progressive_barrier_penalty = 10.0;
      mads_cfg.accelerate_heavy_kernels = false;
      mads_cfg.surrogate_mode = SurrogateMode::Off;

      const auto blackbox = [&](const Series& x) -> BlackboxEvaluation {
        const std::vector<double> host = to_host_f64_vec(x.reshape({x.numel()}));
        const auto [f, g1, g2, g3] = nomad_example1_eval(host);
        BlackboxEvaluation out;
        out.objectives = pluss::make_series(f);
        out.constraints = pluss::make_series(
            std::vector<double>{g1, g2, g3},
            {3},
            Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU));
        out.evaluation_ok = std::isfinite(f);
        out.count_evaluation = true;
        return out;
      };

      const BlackboxAdvancedOutcome pluss_out =
          orthomads_blackbox_minimizer(blackbox, x0, mads_cfg);

      double pluss_obj = pluss_out.scalarized_minimum;
      if (pluss_out.objective_vector.defined() && pluss_out.objective_vector.numel() >= 1) {
        const std::vector<double> obj_vals =
            pluss_out.objective_vector.to(pCPU, ScalarType::Float64).contiguous().to_vec<double>();
        if (!obj_vals.empty()) {
          pluss_obj = obj_vals.front();
        }
      }

      const std::vector<double> x_best = to_host_f64_vec(pluss_out.minimizer.reshape({n_dim}));
      const auto [f_eval, g1, g2, g3] = nomad_example1_eval(x_best);

      std::cerr << "nomad_ref_obj=" << *nomad_obj << " pluss_obj=" << pluss_obj
                << " abs_diff=" << std::abs(pluss_obj - *nomad_obj) << '\n';

      expect(std::isfinite(pluss_obj), "Pluss MADS returned non-finite objective", &failures);
      expect(std::isfinite(f_eval), "Pluss best-point objective is non-finite", &failures);
      expect(g1 <= 1e-8 && g2 <= 1e-8 && g3 <= 1e-8,
             "Pluss MADS best point violates NOMAD example constraints", &failures);
      expect(pluss_obj <= -1800.0, "Pluss MADS objective too weak for NOMAD example1 landscape", &failures);
      expect(std::abs(pluss_obj - *nomad_obj) <= 50.0,
             "Pluss MADS deviates too far from NOMAD reference objective", &failures);
    }
  }

  if (failures != 0) {
    std::cerr << "external_reference_parity_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "external_reference_parity_test: all checks passed\n";
  return 0;
}
