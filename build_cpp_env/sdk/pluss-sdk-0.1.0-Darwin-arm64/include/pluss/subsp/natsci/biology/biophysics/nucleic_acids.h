#pragma once

// Nearest-neighbour thermodynamics of nucleic-acid duplexes (DNA/DNA, RNA/RNA, RNA/DNA): duplex enthalpy / entropy from the sequence,
// salt-corrected melting temperatures and two-state melting curves, batched over lanes of concentrations / ion concentrations / temperature.
//
// The model follows Biopython's open implementation (Bio/SeqUtils/MeltingTemp.py, `Tm_NN` and `salt_correction`, read from the GitHub
// source, SHA-256 4f3cbc58...), whose tables (generated into nucleic_acid_tables.h) cite: Breslauer et al. 1986, Sugimoto et al. 1995/1996,
// Allawi & SantaLucia 1997, SantaLucia & Hicks 2004, Freier et al. 1986, Xia et al. 1998, Chen et al. 2012. For a perfectly paired duplex
//     dH = sum_nn dH_nn + initiation terms (+ symmetry term if self-complementary),  dS likewise,
//     Tm = 1000 dH / (dS + R ln k) - 273.15  [deg C],  k = (c1 - c2/2) 1e-9 (c in nM, non-self-complementary) or c1 1e-9,  R = 1.987 cal/(K mol),
// with the salt corrections 1-7 of that file (Schildkraut & Lifson 1965, Wetmur 1991, SantaLucia et al. 1996 and 1998, Owczarzy et al.
// 2004 and 2008, von Ahsen et al. 2001 sodium-equivalent). Mismatch and dangling-end tables of that file are not implemented here.
// The melting curve (fraction of strands in duplex versus temperature) is the exact two-state equilibrium K(T) = exp(-(dH - T dS)/(R T)).

#include "pluss/subsp/natsci/biology/biophysics/nucleic_acid_tables.h"
#include "pluss/subsp/natsci/biology/dynamics.h"

#include <cmath>
#include <map>
#include <string>

namespace pluss::subsp::natsci::biology::biophysics::nucleic_acids {

namespace dynamics = ::pluss::subsp::natsci::biology::dynamics;
using dynamics::Coeff;
using Table = std::map<std::string, std::pair<double, double>>;

inline constexpr double kGasConstantCalPerKMol = 1.987; // the value used by the reference implementation

struct Duplex final {
  double dh_kcal{0.0};   // before the salt correction
  double ds_cal{0.0};    // cal/(mol K), before the salt correction
  std::int64_t length{0};
  double gc_fraction{0.0};
  bool self_complementary{false};
};

namespace detail {
inline std::string normalise(const std::string &seq) {
  std::string out;
  for (const char ch : seq) {
    if (ch == ' ' || ch == '\t' || ch == '\n' || std::isdigit(static_cast<unsigned char>(ch)))
      continue;
    char u = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (u == 'U')
      u = 'T'; // RNA sequences are back-transcribed exactly as the reference implementation does
    if (u == 'A' || u == 'C' || u == 'G' || u == 'T' || u == 'I')
      out.push_back(u);
  }
  return out;
}
inline char complement(char c) { return c == 'A' ? 'T' : c == 'T' ? 'A' : c == 'G' ? 'C' : c == 'C' ? 'G' : 'I'; }
} // namespace detail

// enthalpy / entropy of the perfectly paired duplex of `sequence` (5'->3') with the nearest-neighbour table `nn`
inline Duplex duplex_thermodynamics(const std::string &sequence, const Table &nn, bool self_complementary = false) {
  const std::string seq = detail::normalise(sequence);
  dynamics::require(seq.size() >= 2, "nucleic_acids: a duplex needs at least two bases");
  std::string comp;
  for (const char c : seq)
    comp.push_back(detail::complement(c));
  Duplex d;
  d.length = static_cast<std::int64_t>(seq.size());
  std::int64_t gc = 0;
  for (const char c : seq)
    gc += (c == 'G' || c == 'C');
  d.gc_fraction = static_cast<double>(gc) / static_cast<double>(seq.size());
  d.self_complementary = self_complementary;
  const auto add = [&](const char *key) {
    const auto &v = nn.at(key);
    d.dh_kcal += v.first;
    d.ds_cal += v.second;
  };
  add("init");
  add(d.gc_fraction == 0.0 ? "init_allA/T" : "init_oneG/C");
  if (seq.front() == 'T')
    add("init_5T/A");
  if (seq.back() == 'A')
    add("init_5T/A");
  const std::string ends{seq.front(), seq.back()};
  for (const char c : ends)
    add((c == 'A' || c == 'T') ? "init_A/T" : "init_G/C");
  for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
    std::string key = seq.substr(i, 2) + "/" + comp.substr(i, 2);
    std::string reversed(key.rbegin(), key.rend());
    const auto it = nn.find(key);
    const auto jt = nn.find(reversed);
    dynamics::require(it != nn.end() || jt != nn.end(), "nucleic_acids: neighbour " + key + " is not in the table");
    const auto &v = it != nn.end() ? it->second : jt->second;
    d.dh_kcal += v.first;
    d.ds_cal += v.second;
  }
  if (self_complementary)
    add("sym");
  return d;
}

// ---- salt corrections (methods 1-7 of the reference implementation); ion concentrations in mM, lanes broadcast ----------------
// methods 1-4: add to Tm (deg C); method 5: add to the entropy (cal/(mol K)); methods 6, 7: add to 1/Tm (1/K)
inline Series salt_correction(const Series &na, const Series &k, const Series &tris, const Series &mg, const Series &dntps, int method, const Duplex &duplex) {
  dynamics::require(method >= 0 && method <= 7, "nucleic_acids: salt correction method must be 0-7");
  Series zero = na.mul(0.0);
  if (method == 0)
    return zero;
  Series mon_mm = na.add(k).add(tris.mul(0.5)); // millimolar
  const Series other = k.add(mg).add(tris).add(dntps);
  const Series eq = other.gt(0.0).logical_and(dntps.lt(mg)).logical_and(zero.add(method != 7 ? 1.0 : 0.0).gt(0.0));
  const Series extra = mg.sub(dntps).clamp(0.0, 1e300).sqrt().mul(120.0);
  mon_mm = eq.where(mon_mm.add(extra), mon_mm);
  const Series mon = mon_mm.mul(1e-3);
  const Series ln_mon = mon.log();
  const double gc = duplex.gc_fraction;
  const double n = static_cast<double>(duplex.length);
  if (method == 1)
    return mon.log10().mul(16.6);
  if (method == 2)
    return mon.div(mon.mul(0.7).add(1.0)).log10().mul(16.6);
  if (method == 3)
    return mon.log10().mul(12.5);
  if (method == 4)
    return mon.log10().mul(11.7);
  if (method == 5)
    return ln_mon.mul(0.368 * (n - 1.0));
  const Series corr6 = ln_mon.mul((4.29 * gc - 3.95) * 1e-5).add(ln_mon.square().mul(9.40e-6));
  if (method == 6)
    return corr6;
  // method 7 (Owczarzy et al. 2008): free magnesium after dNTP binding, then a decision tree on R = sqrt(Mg) / Mon
  Series mgm = mg.mul(1e-3);
  const Series dn = dntps.mul(1e-3);
  const double ka = 3e4;
  const Series q = dn.mul(ka).sub(mgm.mul(ka)).add(1.0);
  const Series free_mg = q.mul(-1.0).add(q.square().add(mgm.mul(4.0 * ka)).sqrt()).div(2.0 * ka);
  mgm = dntps.gt(0.0).where(free_mg, mgm);
  const Series ln_mg = mgm.clamp(1e-300, 1e300).log();
  const Series ratio = mgm.sqrt().div(mon.clamp(1e-300, 1e300));
  const double b = -0.911, c = 6.26, e = -48.2, f = 52.5;
  Series a = zero.add(3.92), d = zero.add(1.42), g = zero.add(8.31);
  const Series mid = ratio.ge(0.22).logical_and(ratio.lt(6.0)).logical_and(mon_mm.gt(0.0));
  const Series a2 = ln_mon.mul(mon.sqrt()).mul(-0.352).add(0.843).mul(3.92);
  const Series d2 = ln_mon.mul(-4.03e-3).add(1.279).sub(ln_mon.square().mul(8.03e-3)).mul(1.42);
  const Series g2 = ln_mon.mul(-0.258).add(0.486).add(ln_mon.pow(3.0).mul(5.25e-3)).mul(8.31);
  a = mid.where(a2, a);
  d = mid.where(d2, d);
  g = mid.where(g2, g);
  const Series full = a.add(ln_mg.mul(b)).add(d.mul(ln_mg).add(c).mul(gc)).add(g.mul(ln_mg.square()).add(ln_mg.mul(f)).add(e).mul(1.0 / (2.0 * (n - 1.0)))).mul(1e-5);
  const Series low = mon_mm.gt(0.0).logical_and(ratio.lt(0.22));
  return low.where(corr6, full);
}

// Melting temperature in deg C. c1, c2: strand concentrations in nM (lanes); for a self-complementary duplex only c1 is used.
inline Series melting_temperature(const Duplex &duplex, const Series &c1_nm, const Series &c2_nm, int salt_method, const Series &na, const Series &k, const Series &tris,
                                  const Series &mg, const Series &dntps) {
  const Series kk = duplex.self_complementary ? c1_nm.mul(1e-9) : c1_nm.sub(c2_nm.mul(0.5)).mul(1e-9);
  const Series corr = salt_correction(na, k, tris, mg, dntps, salt_method, duplex);
  Series ds = kk.mul(0.0).add(duplex.ds_cal);
  if (salt_method == 5)
    ds = ds.add(corr);
  Series tm = ds.add(kk.log().mul(kGasConstantCalPerKMol)).reciprocal().mul(1000.0 * duplex.dh_kcal).sub(273.15);
  if (salt_method >= 1 && salt_method <= 4)
    tm = tm.add(corr);
  if (salt_method == 6 || salt_method == 7)
    tm = tm.add(273.15).reciprocal().add(corr).reciprocal().sub(273.15);
  return tm;
}

// ---- two-state melting curve ----------------------------------------------------------------------------------------------
// K(T) = exp(-(dH - T dS)/(R T)) (1/M); strands A and B at total concentrations ca, cb (M): K (ca - x)(cb - x) = x for the duplex x
// (self-complementary strand: 2 K a^2 ... see below). Returns the fraction of the limiting strand in duplex form; T in K, lanes broadcast.
inline Series equilibrium_constant(const Duplex &duplex, const Series &temperature_k, double salt_entropy_correction = 0.0) {
  const double dh = duplex.dh_kcal * 1000.0, ds = duplex.ds_cal + salt_entropy_correction;
  return temperature_k.mul(ds).mul(-1.0).add(dh).div(temperature_k.mul(-kGasConstantCalPerKMol)).exp();
}
inline Series duplex_fraction(const Series &k, const Series &ca_molar, const Series &cb_molar, bool self_complementary) {
  if (self_complementary) {
    // 2 A <-> D: K = [D]/[A]^2, total strand concentration ca = [A] + 2 [D]  =>  2 K [A]^2 + [A] - ca = 0 ; fraction in duplex = 2 [D] / ca
    const Series a = ca_molar.mul(8.0).mul(k).add(1.0).sqrt().sub(1.0).div(k.mul(4.0));
    return a.div(ca_molar).mul(-1.0).add(1.0);
  }
  // K (ca - x)(cb - x) = x  =>  K x^2 - (1 + K (ca + cb)) x + K ca cb = 0, smaller root
  const Series b = k.mul(ca_molar.add(cb_molar)).add(1.0);
  const Series disc = b.square().sub(k.mul(k).mul(ca_molar).mul(cb_molar).mul(4.0)).clamp(0.0, 1e300).sqrt();
  const Series x = b.sub(disc).div(k.mul(2.0));
  const Series limiting = ca_molar.lt(cb_molar).where(ca_molar, cb_molar);
  return x.div(limiting);
}

// the nearest-neighbour table by name ("DNA_NN3", "DNA_NN4", "RNA_NN2", ...)
inline const Table &table(const std::string &name) {
  if (name == "DNA_NN1") return tables::DNA_NN1;
  if (name == "DNA_NN2") return tables::DNA_NN2;
  if (name == "DNA_NN3") return tables::DNA_NN3;
  if (name == "DNA_NN4") return tables::DNA_NN4;
  if (name == "RNA_NN1") return tables::RNA_NN1;
  if (name == "RNA_NN2") return tables::RNA_NN2;
  if (name == "RNA_NN3") return tables::RNA_NN3;
  if (name == "R_DNA_NN1") return tables::R_DNA_NN1;
  throw std::invalid_argument("nucleic_acids: unknown table " + name);
}

} // namespace pluss::subsp::natsci::biology::biophysics::nucleic_acids
