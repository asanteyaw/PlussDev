#include "pluss/subsp/math/finance/chord.h"

#include "CodeM/core/Errors.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>
#include <bit>
#include <cstring>
#include <map>
#include <sstream>

namespace pluss::subsp::math::finance::module {
namespace {

using pluss::Series;

std::int64_t count_of_(const Series& x) { return x.defined() ? x.numel() : 1; }

std::int64_t length_of_(const Field& f) {
  std::int64_t n = std::max({count_of_(f.instrument.strike), count_of_(f.instrument.maturity), count_of_(f.market.spot), count_of_(f.market.rate),
                             count_of_(f.market.dividend_yield)});
  if (f.instrument.type == pricing::OptionType::Auto) n = std::max(n, count_of_(f.instrument.is_call));
  if (f.model.holds<model::BlackScholes>()) n = std::max(n, count_of_(f.model.as<model::BlackScholes>().volatility));
  return n;
}

// x spread over n entries: a single value is repeated, n values are kept.
Series spread_(const Series& x, std::int64_t n) {
  Series v = x.reshape({x.numel()});
  return v.numel() == n ? v : v.expand({n});
}

std::string bits_(double v) {
  std::ostringstream s;
  s << std::bit_cast<std::uint64_t>(v);
  return s.str();
}

// Two assets can share one call when this key agrees. An empty key means the asset is priced on its own.
std::string key_of_(const Field& f) {
  const auto& c = f.controls;
  if (f.instrument.exercise != pricing::ExerciseStyle::European) return {};
  if (c.engine != Engine::Analytic && c.engine != Engine::Cos && c.engine != Engine::Fft) return {};
  if (c.engine == Engine::Fft && c.fft.return_grid) return {};
  std::ostringstream k;
  k << static_cast<int>(c.engine) << '|' << static_cast<int>(c.risk) << '|' << (c.engine == Engine::Fft ? static_cast<int>(f.instrument.type) : 0) << '|' << static_cast<int>(f.model.kind()) << '|';
  switch (c.engine) {
    case Engine::Analytic: k << c.analytic.with_greeks; break;
    case Engine::Cos:
      k << c.cos.num_terms << ',' << bits_(c.cos.truncation) << ',' << bits_(c.cos.cumulant_step);
      break;
    case Engine::Fft:
      k << bits_(c.fft.vol_risk_premium) << ',' << static_cast<int>(c.fft.mode) << ',' << c.fft.num_fft << ',' << bits_(c.fft.du) << ',' << bits_(c.fft.dk) << ','
        << bits_(c.fft.alpha) << ',' << static_cast<int>(c.fft.quadrature);
      break;
    default: break;
  }
  k << '|';
  const bool cos_sv = c.engine == Engine::Cos && (f.model.holds<model::Heston>() || f.model.holds<model::Bates>());
  if ((c.engine == Engine::Analytic && f.model.holds<model::BlackScholes>()) || cos_sv) {
    k << "vectorised";   // the Black-Scholes closed form and the Heston / Bates COS take parameters per option
  } else {
    const auto p = f.model.parameters().to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    for (double v : p) k << bits_(v) << ',';
  }
  return k.str();
}

// Runs the jobs on up to one thread per core; the first exception thrown by any job is rethrown after all threads have finished.
void run_jobs_(const std::vector<std::function<void()>>& jobs, bool parallel) {
  const std::size_t workers = parallel ? std::min<std::size_t>(jobs.size(), std::max(1u, std::thread::hardware_concurrency())) : 1;
  if (workers <= 1) {
    for (const auto& job : jobs) job();
    return;
  }
  std::atomic<std::size_t> next{0};
  std::exception_ptr failure;
  std::mutex guard;
  std::vector<std::thread> threads;
  for (std::size_t w = 0; w < workers; ++w) {
    threads.emplace_back([&] {
      for (std::size_t j = next++; j < jobs.size(); j = next++) {
        try {
          jobs[j]();
        } catch (...) {
          std::lock_guard<std::mutex> lock(guard);
          if (!failure) failure = std::current_exception();
        }
      }
    });
  }
  for (auto& t : threads) t.join();
  if (failure) std::rethrow_exception(failure);
}

// Monte Carlo Heston / Bates assets merge into one simulation with an asset axis when every option of an asset has the same maturity and a scalar market,
// the controls agree, and no control variate or importance sampling is used. An empty key means the asset is priced on its own.
std::string mc_key_of_(const Field& f) {
  const auto& c = f.controls;
  if (c.engine != Engine::MonteCarlo || f.instrument.exercise != pricing::ExerciseStyle::European) return {};
  if (!f.model.holds<model::Heston>() && !f.model.holds<model::Bates>()) return {};
  if (c.mc.control_variate || c.mc.importance_sampling || c.mc.scheme == volatility::SimScheme::Transition) return {};
  const auto scalar = [](const Series& x) { return x.defined() && x.numel() == 1; };
  if (!scalar(f.market.spot) || !scalar(f.market.rate) || !scalar(f.market.dividend_yield)) return {};
  const auto t = f.instrument.maturity.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
  if (t.size() == 0) return {};
  for (double v : t) if (v != t[0]) return {};
  const auto p = f.model.parameters().to(pluss::pCPU, pluss::ScalarType::Float64);
  if (p.numel() != (f.model.holds<model::Heston>() ? 5 : 8)) return {};
  std::ostringstream k;
  k << "mc|" << static_cast<int>(c.risk) << '|' << bits_(t[0]) << '|' << c.mc.paths << ',' << c.mc.steps << ',' << static_cast<int>(c.mc.scheme) << ',' << static_cast<int>(c.mc.rand_method) << ','
    << c.mc.antithetic << ',' << c.mc.randomized_quasi << ',' << c.mc.sobol_scrambling << ',' << c.mc.seed << ',' << c.mc.offset;
  return k.str();
}

Series slice_of_(const Series& x, std::int64_t begin, std::int64_t end) {
  return x.defined() ? x.slice(0, begin, end) : x;
}

} // namespace

const Field::Result& Chord::Result::operator[](const std::string& name) const {
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name) return results[i];
  }
  PLUSS_CHECK(false, "subsp::math::finance::module::Chord::Result: no asset named '" + name + "'");
  return results.front();
}

Chord& Chord::add(std::string name, Field field) {
  PLUSS_CHECK(!name.empty(), "subsp::math::finance::module::Chord::add: the asset name must not be empty");
  PLUSS_CHECK(!contains(name), "subsp::math::finance::module::Chord::add: there is already an asset named '" + name + "'");
  names_.push_back(std::move(name));
  fields_.push_back(std::move(field));
  return *this;
}

Chord::Chord(std::initializer_list<Entry> assets) { add(pluss::RangeView<Entry>(assets)); }

Chord::Chord(pluss::RangeView<Entry> assets) { add(assets); }

Chord& Chord::add(pluss::RangeView<Entry> assets) {
  // Checked as a whole before anything is added, so a failure leaves the Chord as it was.
  for (std::size_t i = 0; i < assets.size(); ++i) {
    const std::string& name = assets[i].first;
    PLUSS_CHECK(!name.empty(), "subsp::math::finance::module::Chord::add: the asset name must not be empty");
    PLUSS_CHECK(!contains(name), "subsp::math::finance::module::Chord::add: there is already an asset named '" + name + "'");
    for (std::size_t j = 0; j < i; ++j) {
      PLUSS_CHECK(assets[j].first != name, "subsp::math::finance::module::Chord::add: the asset name '" + name + "' appears twice in the list");
    }
  }
  for (const Entry& asset : assets) {
    names_.push_back(asset.first);
    fields_.push_back(asset.second);
  }
  return *this;
}

namespace {
const char* kind_label_(model::Kind k) {
  switch (k) {
    case model::Kind::BlackScholes: return "BlackScholes";
    case model::Kind::Sabr: return "Sabr";
    case model::Kind::Heston: return "Heston";
    case model::Kind::Bates: return "Bates";
    case model::Kind::RoughHeston: return "RoughHeston";
    case model::Kind::RoughBergomi: return "RoughBergomi";
  }
  return "";
}
}  // namespace

Chord Chord::from_model_params(const Parameters& p) {
  constexpr const char* who = "subsp::math::finance::module::Chord::from_model_params: ";
  using K = model::Kind;
  PLUSS_CHECK(!p.spot.empty(), std::string(who) + "the column 'spot' is required");
  PLUSS_CHECK(!p.maturity.empty(), std::string(who) + "the column 'maturity' is required");
  const std::pair<const std::vector<double>*, const char*> named[] = {{&p.spot, "spot"}, {&p.rate, "rate"}, {&p.dividend_yield, "dividend_yield"}, {&p.volatility, "volatility"},
      {&p.alpha0, "alpha0"}, {&p.beta, "beta"}, {&p.nu, "nu"}, {&p.kappa, "kappa"}, {&p.theta, "theta"}, {&p.sigma, "sigma"}, {&p.rho, "rho"}, {&p.v0, "v0"},
      {&p.jump_intensity, "jump_intensity"}, {&p.jump_mean, "jump_mean"}, {&p.jump_vol, "jump_vol"}, {&p.alpha, "alpha"}, {&p.xi, "xi"}, {&p.eta, "eta"},
      {&p.maturity, "maturity"}, {&p.strike, "strike"}};

  std::size_t assets = std::max(p.names.size(), p.kind.size());
  for (const auto& [v, name] : named) assets = std::max(assets, v->size());
  PLUSS_CHECK(assets > 0, std::string(who) + "there are no assets");
  for (const auto& [v, name] : named) {
    PLUSS_CHECK(v->size() <= 1 || v->size() == assets,
                std::string(who) + "the column '" + name + "' has " + std::to_string(v->size()) + " entries; it needs 1 or " + std::to_string(assets));
  }
  PLUSS_CHECK(p.kind.size() <= 1 || p.kind.size() == assets, std::string(who) + "kind has " + std::to_string(p.kind.size()) + " entries; it needs 1 or " + std::to_string(assets));
  PLUSS_CHECK(p.names.empty() || p.names.size() == assets, std::string(who) + "names has " + std::to_string(p.names.size()) + " entries; it needs " + std::to_string(assets));

  const std::size_t jumps = (!p.jump_intensity.empty()) + (!p.jump_mean.empty()) + (!p.jump_vol.empty());
  PLUSS_CHECK(!p.kind.empty() || jumps == 0 || jumps == 3, std::string(who) + "jump_intensity, jump_mean and jump_vol must be given together (all three for Bates, none for Heston)");
  std::vector<K> kinds(assets, jumps == 3 ? K::Bates : K::Heston);
  if (p.kind.size() == 1) kinds.assign(assets, p.kind.front());
  else if (p.kind.size() == assets) kinds = p.kind;
  const auto name_of = [&](std::size_t i) { return p.names.empty() ? "asset" + std::to_string(i + 1) : p.names[i]; };

  std::vector<const std::vector<double>*> read;
  // The entry of a column for one asset, read because that asset's model uses it.
  const auto entry = [&](const std::vector<double>& v, const char* name, std::size_t i) {
    read.push_back(&v);
    PLUSS_CHECK(!v.empty(), std::string(who) + "the column '" + name + "' is required (the " + kind_label_(kinds[i]) + " model of asset '" + name_of(i) + "' uses it)");
    const double x = v.size() == 1 ? v.front() : v[i];
    PLUSS_CHECK(std::isfinite(x), std::string(who) + "entry " + std::to_string(i) + " of the column '" + name + "' (asset '" + name_of(i) + "') must be finite");
    return x;
  };
  const auto optional = [&](const std::vector<double>& v, std::size_t i, double fallback) {
    read.push_back(&v);
    return v.empty() ? fallback : (v.size() == 1 ? v.front() : v[i]);
  };

  const auto matrix_ok = [&](const pluss::codem::Series& m, const char* name) {
    PLUSS_CHECK(!m.defined() || (m.sizes().size() == 2 && m.sizes()[0] == static_cast<std::int64_t>(assets) && m.sizes()[1] == static_cast<std::int64_t>(assets)),
                std::string(who) + name + " must be an [" + std::to_string(assets) + ", " + std::to_string(assets) + "] matrix, one row and column per asset");
  };
  matrix_ok(p.price_corr, "price_corr");
  matrix_ok(p.var_corr, "var_corr");
  matrix_ok(p.price_var_cross, "price_var_cross");

  Chord chord;
  for (std::size_t i = 0; i < assets; ++i) {
    Field f;
    const double spot = entry(p.spot, "spot", i);
    f.instrument = instrument::make(pricing::Call, p.strike.empty() ? spot : optional(p.strike, i, spot), optional(p.maturity, i, 1.0));
    f.market = market::make(spot, optional(p.rate, i, 0.0), optional(p.dividend_yield, i, 0.0));
    std::vector<double> params;
    switch (kinds[i]) {
      case K::BlackScholes: params = {entry(p.volatility, "volatility", i)}; break;
      case K::Sabr: params = {entry(p.alpha0, "alpha0", i), entry(p.beta, "beta", i), entry(p.nu, "nu", i), entry(p.rho, "rho", i)}; break;
      case K::Heston:
      case K::Bates:
        params = {entry(p.kappa, "kappa", i), entry(p.theta, "theta", i), entry(p.sigma, "sigma", i), entry(p.rho, "rho", i), entry(p.v0, "v0", i)};
        if (kinds[i] == K::Bates) {
          params.push_back(entry(p.jump_intensity, "jump_intensity", i));
          params.push_back(entry(p.jump_mean, "jump_mean", i));
          params.push_back(entry(p.jump_vol, "jump_vol", i));
        }
        break;
      case K::RoughHeston:
        params = {entry(p.kappa, "kappa", i), entry(p.theta, "theta", i), entry(p.sigma, "sigma", i), entry(p.alpha, "alpha", i), entry(p.rho, "rho", i), entry(p.v0, "v0", i)};
        break;
      case K::RoughBergomi: params = {entry(p.xi, "xi", i), entry(p.eta, "eta", i), entry(p.alpha, "alpha", i), entry(p.rho, "rho", i)}; break;
    }
    f.model = model::make(kinds[i], params);
    f.controls = p.controls;
    chord.add(name_of(i), std::move(f));
  }
  // a column that no asset's model reads is refused, so a misspelt or misplaced column is not silently dropped
  for (const auto& [v, name] : named) {
    if (v->empty() || v == &p.maturity || v == &p.strike || v == &p.rate || v == &p.dividend_yield) continue;
    PLUSS_CHECK(std::find(read.begin(), read.end(), v) != read.end(), std::string(who) + "the column '" + name + "' is not used by the model of any asset");
  }
  chord.price_corr = p.price_corr;
  chord.var_corr = p.var_corr;
  chord.price_var_cross = p.price_var_cross;
  return chord;
}

bool Chord::contains(const std::string& name) const { return std::find(names_.begin(), names_.end(), name) != names_.end(); }

Field& Chord::at(const std::string& name) {
  for (std::size_t i = 0; i < names_.size(); ++i) {
    if (names_[i] == name) return fields_[i];
  }
  PLUSS_CHECK(false, "subsp::math::finance::module::Chord::at: no asset named '" + name + "'");
  return fields_.front();
}

const Field& Chord::at(const std::string& name) const { return const_cast<Chord*>(this)->at(name); }

void Chord::validate() const {
  std::string problems;
  for (std::size_t i = 0; i < fields_.size(); ++i) {
    try {
      fields_[i].validate();
    } catch (const std::exception& e) {
      std::string m = e.what();
      problems += "asset '" + names_[i] + "': " + m.substr(0, m.find("Triggered")) + "\n";
    }
  }
  PLUSS_CHECK(problems.empty(), "subsp::math::finance::module::Chord:\n" + problems);
}

std::size_t Chord::batches() const {
  std::map<std::string, int> groups;
  std::size_t singles = 0;
  for (const auto& f : fields_) {
    const std::string mk = mc_key_of_(f);
    if (!mk.empty()) { groups[mk] = 1; continue; }
    const std::string k = key_of_(f);
    if (k.empty()) ++singles; else groups[k] = 1;
  }
  return singles + groups.size();
}

Chord::Result Chord::price(pluss::codem::Config cfg) const {
  validate();
  Result out;
  out.names = names_;
  out.results.resize(fields_.size());

  // Every call is a job that writes only its own members' results, so the jobs are independent and can run on any thread.
  std::vector<std::function<void()>> jobs;
  std::map<std::string, std::vector<std::size_t>> groups;
  std::map<std::string, std::vector<std::size_t>> mc_groups;
  for (std::size_t i = 0; i < fields_.size(); ++i) {
    const std::string mk = mc_key_of_(fields_[i]);
    if (!mk.empty()) {
      mc_groups[mk].push_back(i);
      continue;
    }
    const std::string k = key_of_(fields_[i]);
    if (k.empty() || fields_.size() == 1) {
      jobs.push_back([this, &out, i, cfg] { out.results[i] = fields_[i].price(cfg); });
    } else {
      groups[k].push_back(i);
    }
  }

  for (const auto& entry : groups) {
    const std::vector<std::size_t>& members = entry.second;
    if (members.size() == 1) {
      jobs.push_back([this, &out, i = members[0], cfg] { out.results[i] = fields_[i].price(cfg); });
      continue;
    }
    jobs.push_back([this, &out, &members, cfg] {
    // merge: every member spread to its own length, then laid end to end
    std::vector<std::int64_t> lengths;
    std::vector<Series> strike, maturity, spot, rate, yield, is_call, vol;
    const bool black_scholes = fields_[members[0]].model.holds<model::BlackScholes>() && fields_[members[0]].controls.engine == Engine::Analytic;
    const bool is_heston = fields_[members[0]].model.holds<model::Heston>(), is_bates = fields_[members[0]].model.holds<model::Bates>();
    std::vector<Series> kappa, theta, sigma, rho, v0, lambda, jump_mean, jump_vol;
    bool auto_type = false;
    for (std::size_t m : members) auto_type = auto_type || fields_[m].instrument.type != fields_[members[0]].instrument.type;
    auto_type = auto_type || fields_[members[0]].instrument.type == pricing::OptionType::Auto;
    for (std::size_t m : members) {
      const Field& f = fields_[m];
      const std::int64_t n = length_of_(f);
      lengths.push_back(n);
      strike.push_back(spread_(f.instrument.strike, n));
      maturity.push_back(spread_(f.instrument.maturity, n));
      spot.push_back(spread_(f.market.spot, n));
      rate.push_back(spread_(f.market.rate, n));
      yield.push_back(spread_(f.market.dividend_yield, n));
      if (auto_type) {
        const auto flag = [&](double v) { return pluss::series::full({1}, v, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)); };
        is_call.push_back(spread_(f.instrument.type == pricing::OptionType::Auto ? f.instrument.is_call : flag(f.instrument.type == pricing::OptionType::Call ? 1.0 : 0.0), n));
      }
      if (black_scholes) vol.push_back(spread_(f.model.as<model::BlackScholes>().volatility, n));
      const auto sv = [&](const auto& m) {
        kappa.push_back(spread_(m.kappa, n));
        theta.push_back(spread_(m.theta, n));
        sigma.push_back(spread_(m.sigma, n));
        rho.push_back(spread_(m.rho, n));
        v0.push_back(spread_(m.v0, n));
      };
      if (is_heston) sv(f.model.as<model::Heston>());
      if (is_bates) {
        const auto& m = f.model.as<model::Bates>();
        sv(m);
        const auto given = [&](const Series& x, double fallback) { return x.defined() ? x : pluss::series::full({1}, fallback, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)); };
        lambda.push_back(spread_(given(m.jump_intensity, 0.15), n));
        jump_mean.push_back(spread_(given(m.jump_mean, -0.08), n));
        jump_vol.push_back(spread_(given(m.jump_vol, 0.20), n));
      }
    }
    const auto join = [](const std::vector<Series>& parts) {
      return pluss::unite(pluss::msharp::Lister::Series(parts), 0);
    };
    Field merged = fields_[members[0]];
    merged.instrument.strike = join(strike);
    merged.instrument.maturity = join(maturity);
    merged.market.spot = join(spot);
    merged.market.rate = join(rate);
    merged.market.dividend_yield = join(yield);
    if (auto_type) {
      merged.instrument.type = pricing::OptionType::Auto;
      merged.instrument.is_call = join(is_call);
    }
    if (black_scholes) merged.model = model::BlackScholes{.volatility = join(vol)};
    if (is_heston) merged.model = model::Heston{.kappa = join(kappa), .theta = join(theta), .sigma = join(sigma), .rho = join(rho), .v0 = join(v0)};
    if (is_bates) {
      merged.model = model::Bates{.kappa = join(kappa), .theta = join(theta), .sigma = join(sigma), .rho = join(rho), .v0 = join(v0),
                                  .jump_intensity = join(lambda), .jump_mean = join(jump_mean), .jump_vol = join(jump_vol)};
    }
    const pricer::Result all = merged.price(cfg);

    std::int64_t begin = 0;
    for (std::size_t j = 0; j < members.size(); ++j) {
      const std::int64_t end = begin + lengths[j];
      pricer::Result r;
      r.price = slice_of_(all.price, begin, end);
      r.greeks.delta = slice_of_(all.greeks.delta, begin, end);
      r.greeks.gamma = slice_of_(all.greeks.gamma, begin, end);
      r.greeks.vega = slice_of_(all.greeks.vega, begin, end);
      r.greeks.theta = slice_of_(all.greeks.theta, begin, end);
      r.greeks.rho = slice_of_(all.greeks.rho, begin, end);
      out.results[members[j]] = std::move(r);
      begin = end;
    }
    });
  }

  for (const auto& entry : mc_groups) {
    const std::vector<std::size_t>& members = entry.second;
    if (members.size() == 1) {
      jobs.push_back([this, &out, i = members[0], cfg] { out.results[i] = fields_[i].price(cfg); });
      continue;
    }
    jobs.push_back([this, &out, &members, cfg] {
      const auto host1 = [](const Series& x) { return x.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>().front(); };
      std::vector<pricing::detail::McAsset> assets;
      std::vector<std::size_t> counts;
      for (std::size_t m : members) {
        const Field& f = fields_[m];
        pricing::detail::McAsset a;
        const auto take = [&](const auto& md) {
          a.params.kappa = host1(md.kappa); a.params.theta = host1(md.theta); a.params.sigma = host1(md.sigma); a.params.rho = host1(md.rho); a.params.v0 = host1(md.v0);
        };
        if (f.model.holds<model::Heston>()) {
          take(f.model.as<model::Heston>());
        } else {
          const auto& md = f.model.as<model::Bates>();
          take(md);
          a.bates = true;
          a.params.jump_intensity = md.jump_intensity.defined() ? host1(md.jump_intensity) : 0.15;
          a.params.jump_mean = md.jump_mean.defined() ? host1(md.jump_mean) : -0.08;
          a.params.jump_vol = md.jump_vol.defined() ? host1(md.jump_vol) : 0.20;
        }
        a.spot = host1(f.market.spot); a.rate = host1(f.market.rate); a.dividend_yield = host1(f.market.dividend_yield);
        const std::int64_t n = length_of_(f);
        const auto strikes = spread_(f.instrument.strike, n).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
        a.strikes.assign(strikes.begin(), strikes.end());
        if (f.instrument.type == pricing::OptionType::Auto) {
          const auto flags = spread_(f.instrument.is_call, n).to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
          for (double v : flags) a.is_call.push_back(v >= 0.5);
        } else {
          a.is_call.assign(static_cast<std::size_t>(n), f.instrument.type == pricing::OptionType::Call);
        }
        counts.push_back(static_cast<std::size_t>(n));
        assets.push_back(std::move(a));
      }
      const Field& first = fields_[members[0]];
      const double maturity = first.instrument.maturity.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>()[0];
      pricing::MonteCarloOptions opt;
      opt.paths = first.controls.mc.paths; opt.steps = first.controls.mc.steps; opt.scheme = first.controls.mc.scheme; opt.rand_method = first.controls.mc.rand_method;
      opt.randomized_quasi = first.controls.mc.randomized_quasi; opt.sobol_scrambling = first.controls.mc.sobol_scrambling; opt.antithetic = first.controls.mc.antithetic;
      opt.seed = first.controls.mc.seed; opt.offset = first.controls.mc.offset;
      const bool greeks = first.controls.risk == Risk::All;
      const Config f64 = Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform);
      const auto res = pricing::detail::sv_mc_assets(assets, maturity, opt, greeks, f64);
      for (std::size_t j = 0; j < members.size(); ++j) {
        const auto make = [&](const std::vector<double>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, f64); };
        pricer::Result r;
        r.price = make(res[j].price);
        if (greeks) {
          r.greeks.delta = make(res[j].delta); r.greeks.gamma = make(res[j].gamma); r.greeks.vega = make(res[j].vega);
          r.greeks.theta = make(res[j].theta); r.greeks.rho = make(res[j].rho);
        }
        out.results[members[j]] = std::move(r);
      }
    });
  }

  run_jobs_(jobs, parallel);

  std::vector<Series> prices;
  out.offsets.push_back(0);
  for (const auto& r : out.results) {
    prices.push_back(r.price.reshape({r.price.numel()}));
    out.offsets.push_back(out.offsets.back() + r.price.numel());
  }
  if (!prices.empty()) out.price = pluss::unite(pluss::msharp::Lister::Series(prices), 0);
  return out;
}

Chord::Paths Chord::simulate_paths(pluss::codem::Config cfg) const {
  constexpr const char* who = "subsp::math::finance::module::Chord::simulate_paths: ";
  PLUSS_CHECK(!fields_.empty(), std::string(who) + "the Chord has no assets");
  validate();
  const auto host1 = [](const Series& x) { return x.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>().front(); };
  const Config f64 = Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(cfg.platform);
  const auto& mc0 = fields_[0].controls.mc;
  double horizon = 0.0;
  std::vector<double> spot, growth, kappa, theta, sigma, rho, v0, lambda, jmean, jvol, bs_vol, alpha0, beta, nu, ralpha, xi_v, eta_v;
  std::vector<volatility::detail::MultiKind> kind;
  bool any_jumps = false, any_non_sv = false;
  for (std::size_t i = 0; i < fields_.size(); ++i) {
    const Field& f = fields_[i];
    const auto& mc = f.controls.mc;
    PLUSS_CHECK(mc.paths == mc0.paths && mc.steps == mc0.steps && mc.scheme == mc0.scheme && mc.rand_method == mc0.rand_method && mc.antithetic == mc0.antithetic &&
                    mc.randomized_quasi == mc0.randomized_quasi && mc.sobol_scrambling == mc0.sobol_scrambling && mc.seed == mc0.seed && mc.offset == mc0.offset,
                std::string(who) + "asset '" + names_[i] + "' must have the same controls.mc (paths, steps, scheme, random numbers, seed) as the first asset");
    const auto maturity = f.instrument.maturity.to(pluss::pCPU, pluss::ScalarType::Float64).host<double>();
    for (double m : maturity) horizon = std::max(horizon, m);
    spot.push_back(host1(f.market.spot));
    growth.push_back(host1(f.market.rate) - host1(f.market.dividend_yield));
    // filler for the columns an asset does not use, so that every column has one entry per asset
    double k = 1.0, th = 1.0, sg = 1.0, r = 0.0, v = 1.0, lam = 0.0, jm = 0.0, jv = 0.0, bv = 0.0, a0 = 1.0, be = 1.0, n = 0.0, ra = -0.25, xv = 1.0, ev = 1.0;
    volatility::detail::MultiKind kd = volatility::detail::MultiKind::Sv;
    const auto take = [&](const auto& m) {
      k = host1(m.kappa); th = host1(m.theta); sg = host1(m.sigma); r = host1(m.rho); v = host1(m.v0);
    };
    if (f.model.holds<model::Heston>()) {
      take(f.model.as<model::Heston>());
    } else if (f.model.holds<model::Bates>()) {
      const auto& m = f.model.as<model::Bates>();
      take(m);
      const auto given = [&](const Series& x, double d) { return x.defined() ? host1(x) : d; };
      lam = given(m.jump_intensity, 0.15); jm = given(m.jump_mean, -0.08); jv = given(m.jump_vol, 0.20);
      any_jumps = true;
    } else if (f.model.holds<model::BlackScholes>()) {
      const auto& m = f.model.as<model::BlackScholes>();
      PLUSS_CHECK(m.volatility.defined() && m.volatility.numel() == 1,
                  std::string(who) + "asset '" + names_[i] + "' needs a single Black-Scholes volatility to be simulated (it has " + std::to_string(m.volatility.defined() ? m.volatility.numel() : 0) + ")");
      bv = host1(m.volatility);
      kd = volatility::detail::MultiKind::BlackScholes;
      any_non_sv = true;
    } else if (f.model.holds<model::Sabr>()) {
      const auto& m = f.model.as<model::Sabr>();
      a0 = host1(m.alpha0); be = host1(m.beta); n = host1(m.nu); r = host1(m.rho);
      kd = volatility::detail::MultiKind::Sabr;
      any_non_sv = true;
    } else if (f.model.holds<model::RoughHeston>()) {
      const auto& m = f.model.as<model::RoughHeston>();
      take(m);
      ra = host1(m.alpha);
      kd = volatility::detail::MultiKind::RoughHeston;
      any_non_sv = true;
    } else if (f.model.holds<model::RoughBergomi>()) {
      const auto& m = f.model.as<model::RoughBergomi>();
      xv = host1(m.xi); ev = host1(m.eta); ra = host1(m.alpha); r = host1(m.rho);
      kd = volatility::detail::MultiKind::RoughBergomi;
      any_non_sv = true;
    } else {
      PLUSS_CHECK(false, std::string(who) + "asset '" + names_[i] + "' holds " + f.model.name() + "; its model cannot be simulated together with others");
    }
    kappa.push_back(k); theta.push_back(th); sigma.push_back(sg); rho.push_back(r); v0.push_back(v);
    lambda.push_back(lam); jmean.push_back(jm); jvol.push_back(jv); bs_vol.push_back(bv); alpha0.push_back(a0); beta.push_back(be); nu.push_back(n);
    ralpha.push_back(ra); xi_v.push_back(xv); eta_v.push_back(ev);
    kind.push_back(kd);
  }
  PLUSS_CHECK(horizon > 0.0, std::string(who) + "the longest maturity must be positive");

  // A Black-Scholes asset has no second factor: its row and column of var_corr must be the identity's and the cross correlations with its (absent) variance shock zero.
  const auto host_matrix = [&](const Series& m, const char* name) -> std::vector<double> {
    if (!m.defined()) return {};
    PLUSS_CHECK(m.sizes().size() == 2 && m.sizes()[0] == static_cast<std::int64_t>(fields_.size()) && m.sizes()[1] == static_cast<std::int64_t>(fields_.size()),
                std::string(who) + name + " must be an [assets, assets] matrix");
    const auto h = m.to(pluss::pCPU, pluss::ScalarType::Float64).contiguous().host<double>();
    return std::vector<double>(h.begin(), h.end());
  };
  if (any_non_sv) {
    const std::size_t A = fields_.size();
    const auto v = host_matrix(var_corr, "var_corr"), c = host_matrix(price_var_cross, "price_var_cross");
    for (std::size_t i = 0; i < A; ++i) {
      if (kind[i] != volatility::detail::MultiKind::BlackScholes) continue;
      for (std::size_t j = 0; j < A; ++j) {
        if (j == i) continue;
        PLUSS_CHECK(v.empty() || (v[i * A + j] == 0.0 && v[j * A + i] == 0.0),
                    std::string(who) + "var_corr[" + names_[i] + ", " + names_[j] + "] must be 0: '" + names_[i] + "' is a Black-Scholes asset and has no variance shock");
        PLUSS_CHECK(c.empty() || c[j * A + i] == 0.0,
                    std::string(who) + "price_var_cross[" + names_[j] + ", " + names_[i] + "] must be 0: '" + names_[i] + "' is a Black-Scholes asset and has no variance shock");
      }
    }
  }

  const auto S = [&](const std::vector<double>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}, f64); };
  volatility::detail::MultiSvParams p;
  p.kind = kind;
  p.bs_vol = S(bs_vol); p.alpha0 = S(alpha0); p.beta = S(beta); p.nu = S(nu); p.rough_alpha = S(ralpha); p.xi = S(xi_v); p.eta = S(eta_v);
  p.spot0 = S(spot); p.growth = S(growth); p.kappa = S(kappa); p.theta = S(theta); p.sigma = S(sigma); p.rho = S(rho); p.v0 = S(v0);
  if (any_jumps) { p.jump_intensity = S(lambda); p.jump_mean = S(jmean); p.jump_vol = S(jvol); }
  p.price_correlation = price_corr;
  p.variance_correlation = var_corr;
  p.price_variance_correlation = price_var_cross;
  volatility::SimOptions sim;
  sim.scheme = mc0.scheme; sim.method = mc0.rand_method; sim.randomized_quasi = mc0.randomized_quasi; sim.sobol_scrambling = mc0.sobol_scrambling;
  sim.antithetic = mc0.antithetic; sim.seed = mc0.seed; sim.offset = mc0.offset;
  const auto r = volatility::detail::multi_sv_paths(mc0.steps, mc0.paths, horizon / static_cast<double>(mc0.steps), p, sim, f64, mc0.seed);
  Paths out;
  out.names = names_;
  out.asset = r.spot; out.variance = r.variance; out.asset_shocks = r.innovation_spot; out.vol_shocks = r.innovation_var;
  out.horizon = horizon;
  return out;
}

std::vector<pluss::codem::Series> Chord::implied_vol(const pricing::ImpliedVolOptions& options, pluss::codem::Config cfg) const {
  const Result priced = price(cfg);
  std::vector<Series> out;
  for (std::size_t i = 0; i < fields_.size(); ++i) out.push_back(fields_[i].implied_vol(priced.results[i].price, options, cfg));
  return out;
}

} // namespace pluss::subsp::math::finance::module
