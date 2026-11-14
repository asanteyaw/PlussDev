#pragma once

#include "pluss/subsp/math/finance/module.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::finance::module {

// Coordinates several single-asset Fields, each with its own market, model and engine, and runs the same operation across all of them.
// Assets that share an engine, controls and model parameters (and, for Black-Scholes with the analytic engine, even different volatilities) are
// merged into one vectorised call; the others are priced one by one.
class Chord {
public:
  // What price() returns: one Field::Result per asset, in the order the assets were added, and the prices of all assets laid end to end.
  struct Result {
    std::vector<std::string> names;
    std::vector<Field::Result> results;
    pluss::codem::Series price = pluss::series::undefined();   // every asset's prices, concatenated
    std::vector<std::int64_t> offsets;                           // asset i owns price[offsets[i], offsets[i + 1])

    // The result of one asset, by name; throws if there is no such asset.
    [[nodiscard]] CODEM_API const Field::Result& operator[](const std::string& name) const;
  };

  // Simulated paths of all assets of a Chord, laid out [paths, steps + 1, assets] (the shocks [paths, steps, assets]); asset i is names[i].
  struct Paths {
    std::vector<std::string> names;
    pluss::codem::Series asset = pluss::series::undefined();
    pluss::codem::Series variance = pluss::series::undefined();
    pluss::codem::Series asset_shocks = pluss::series::undefined();
    pluss::codem::Series vol_shocks = pluss::series::undefined();
    double horizon = 0.0;   // the time of the last column, in years
  };

  // Run the calls of a price() on several threads (one call per task, as many threads as cores). The results do not depend on it; turn it off to
  // price on the calling thread.
  bool parallel = true;

  // One asset of a dictionary: its unique, non-empty name and its Field. A list of entries keeps its order, and the order matters: the correlation
  // matrices of simulate_paths() are indexed by the order the assets were added.
  using Entry = std::pair<std::string, Field>;

  Chord() = default;
  // Builds a Chord from a dictionary of assets, without any add(): Chord book{{"spx", spx}, {"ndx", ndx}}.
  CODEM_API Chord(std::initializer_list<Entry> assets);
  // The same from any contiguous list of entries (a std::vector, std::array or span of them); the list is only read.
  CODEM_API explicit Chord(pluss::RangeView<Entry> assets);

  // Adds an asset under a unique, non-empty name; returns *this.
  CODEM_API Chord& add(std::string name, Field field);
  // Adds several assets at once, in the order given: chord.add({{"spx", spx}, {"ndx", ndx}}), or chord.add(entries) for a std::vector of entries. All or none
  // are added: if any name is empty, repeated in the list or already present, the Chord is left unchanged. Returns *this.
  CODEM_API Chord& add(pluss::RangeView<Entry> assets);

  // The model parameters of several assets as columns, one entry per asset, to build a Chord without writing a Field for each. A column with a single entry applies
  // to every asset. The number of assets is the length of the longest column (and of names, if given); every other column must have one entry or that many.
  //
  // The model of each asset is `kind` (one entry applies to all). Without it the assets are Heston, or Bates when the three jump columns are given. Each model reads
  // these columns, and an entry of a column is read only for the assets whose model uses it (the other entries are ignored; NaN is the customary filler):
  //   BlackScholes  volatility                              Sabr           alpha0, beta, nu, rho (spot is the forward)
  //   Heston        kappa, theta, sigma, rho, v0            Bates          the Heston columns and jump_intensity, jump_mean, jump_vol
  //   RoughHeston   kappa, theta, sigma, alpha, rho, v0     RoughBergomi   xi, eta, alpha, rho
  // A column that no asset's model uses is refused, so a misspelled or misplaced column is not ignored silently.
  struct Parameters {
    std::vector<std::string> names;          // optional; default "asset1", "asset2", ...
    std::vector<model::Kind> kind;           // the model of each asset; empty means Heston (Bates with the jump columns)
    std::vector<double> spot;                // required: the asset prices today (for Sabr, the forward)
    std::vector<double> rate;                // the risk-free rate; default 0
    std::vector<double> dividend_yield;      // default 0
    std::vector<double> volatility;          // BlackScholes
    std::vector<double> alpha0, beta, nu;    // Sabr
    std::vector<double> kappa, theta, sigma, rho, v0;   // Heston, Bates, RoughHeston (rho: also Sabr and RoughBergomi)
    std::vector<double> jump_intensity, jump_mean, jump_vol;   // Bates
    std::vector<double> alpha;               // RoughHeston and RoughBergomi: the Hurst exponent minus 1/2, in (-0.5, 0)
    std::vector<double> xi, eta;             // RoughBergomi
    std::vector<double> maturity{1.0};       // the horizon of each asset in years; the Chord simulates to the longest
    std::vector<double> strike;              // the strike of the call each Field carries for price(); default at the money (the spot)
    Controls controls{.engine = Engine::MonteCarlo};   // shared by every asset; simulate_paths needs equal controls.mc
    pluss::codem::Series price_corr = pluss::series::undefined();            // see the members of the same name below
    pluss::codem::Series var_corr = pluss::series::undefined();
    pluss::codem::Series price_var_cross = pluss::series::undefined();
  };

  // A Chord from a table of model parameters; the correlations of the table are set on the result. Throws, naming the column, for a column a model needs and the table
  // lacks, a column of the wrong length, a column no model uses, jump columns given only in part, or a correlation matrix that is not [assets, assets].
  [[nodiscard]] CODEM_API static Chord from_model_params(const Parameters& parameters);

  [[nodiscard]] std::size_t size() const { return fields_.size(); }
  [[nodiscard]] const std::vector<std::string>& names() const { return names_; }
  [[nodiscard]] CODEM_API bool contains(const std::string& name) const;
  CODEM_API Field& at(const std::string& name);
  [[nodiscard]] CODEM_API const Field& at(const std::string& name) const;
  Field& operator[](const std::string& name) { return at(name); }
  const Field& operator[](const std::string& name) const { return at(name); }

  // Validates every asset and reports all problems, each prefixed with its asset's name.
  CODEM_API void validate() const;

  // Prices every asset.
  [[nodiscard]] CODEM_API Result price(pluss::codem::Config cfg = {}) const;

  // The Black-Scholes implied volatilities of every asset's own model prices, one Series per asset in the order they were added.
  [[nodiscard]] CODEM_API std::vector<pluss::codem::Series> implied_vol(const pricing::ImpliedVolOptions& options = {},
                                                                        pluss::codem::Config cfg = {}) const;

  // Correlation between the assets, used by simulate_paths(); the assets are indexed in the order they were added and an undefined matrix means independent.
  //   price_corr      [assets, assets]  the correlation of the price shocks of the assets
  //   var_corr        [assets, assets]  the correlation of the variance shocks of the assets
  //   price_var_cross [assets, assets]  the cross correlation of the price shock of asset a with the variance shock of asset b (the diagonal is each asset's own rho)
  // When price_var_cross is undefined it is built from the other two and the rho of each asset: each variance shock is rho times its own price shock plus an
  // independent remainder, so corr(price a, variance b) = rho_b * price_corr[a][b], and the remainders carry whatever correlation makes the variance shocks
  // correlate by var_corr (it must be one a valid correlation matrix allows, otherwise simulate_paths() says so). With var_corr undefined the remainders are
  // independent. When price_var_cross is given, the four blocks together must form one valid 2A x 2A correlation matrix.
  pluss::codem::Series price_corr = pluss::series::undefined();
  pluss::codem::Series var_corr = pluss::series::undefined();
  pluss::codem::Series price_var_cross = pluss::series::undefined();

  // Simulates all assets together, correlated by price_corr, var_corr and price_var_cross, in one vectorised pass. The assets may be BlackScholes, Heston, Bates or
  // Sabr (a Black-Scholes asset has no second factor; a Sabr asset's path is its forward and its variance is alpha squared); RoughHeston and RoughBergomi keep a history of their shocks, so their memory grows as paths x steps. The assets
  // must agree on paths, steps, scheme, random-number settings and seed (set them in controls.mc), but may have any engine; the horizon is the longest maturity.
  // The marginal law of each asset is the one its Field simulates; correlation only matters for payoffs on several assets.
  [[nodiscard]] CODEM_API Paths simulate_paths(pluss::codem::Config cfg = {}) const;

  // The number of groups price() would run as one call each: a measure of how much batching applies.
  [[nodiscard]] CODEM_API std::size_t batches() const;

private:
  std::vector<std::string> names_;
  std::vector<Field> fields_;
};

} // namespace pluss::subsp::math::finance::module
