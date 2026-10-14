#pragma once

// Hydrogen-oxygen reference mechanism: Cantera `data/h2o2.yaml` (GitHub main,
// fetched 2026-10-13 through a text-extraction tool; units cm, mol, s,
// cal/mol). Transcription is cross-checked in kinetics_test.cpp by NASA
// polynomial continuity at the mid temperature and by NIST/JANAF values; the
// rate parameters are not independently verified against the YAML file.

#include "pluss/ma/natsci/chemistry.h"

namespace h2o2 {

namespace kin = pluss::subsp::natsci::chemistry::kinetics;
using pluss::subsp::natsci::chemistry::thermo::SpeciesData;

inline std::vector<SpeciesData> species() {
  const auto sp = [](const char *name, std::vector<std::pair<std::string, double>> comp, double tmin,
                     double tmid, double tmax, std::array<double, 7> lo, std::array<double, 7> hi) {
    SpeciesData s;
    s.name = name;
    s.composition = std::move(comp);
    s.t_min = tmin; s.t_mid = tmid; s.t_max = tmax;
    s.low = lo; s.high = hi;
    return s;
  };
  return {
      sp("H2", {{"H", 2}}, 200, 1000, 3500,
         {2.34433112, 7.98052075e-03, -1.9478151e-05, 2.01572094e-08, -7.37611761e-12, -917.935173, 0.683010238},
         {3.3372792, -4.94024731e-05, 4.99456778e-07, -1.79566394e-10, 2.00255376e-14, -950.158922, -3.20502331}),
      sp("H", {{"H", 1}}, 200, 1000, 3500,
         {2.5, 7.05332819e-13, -1.99591964e-15, 2.30081632e-18, -9.27732332e-22, 2.54736599e+04, -0.446682853},
         {2.50000001, -2.30842973e-11, 1.61561948e-14, -4.73515235e-18, 4.98197357e-22, 2.54736599e+04, -0.446682914}),
      sp("O", {{"O", 1}}, 200, 1000, 3500,
         {3.1682671, -3.27931884e-03, 6.64306396e-06, -6.12806624e-09, 2.11265971e-12, 2.91222592e+04, 2.05193346},
         {2.56942078, -8.59741137e-05, 4.19484589e-08, -1.00177799e-11, 1.22833691e-15, 2.92175791e+04, 4.78433864}),
      sp("O2", {{"O", 2}}, 200, 1000, 3500,
         {3.78245636, -2.99673416e-03, 9.84730201e-06, -9.68129509e-09, 3.24372837e-12, -1063.94356, 3.65767573},
         {3.28253784, 1.48308754e-03, -7.57966669e-07, 2.09470555e-10, -2.16717794e-14, -1088.45772, 5.45323129}),
      sp("OH", {{"O", 1}, {"H", 1}}, 200, 1000, 3500,
         {3.99201543, -2.40131752e-03, 4.61793841e-06, -3.88113333e-09, 1.3641147e-12, 3615.08056, -0.103925458},
         {3.09288767, 5.48429716e-04, 1.26505228e-07, -8.79461556e-11, 1.17412376e-14, 3858.657, 4.4766961}),
      sp("H2O", {{"H", 2}, {"O", 1}}, 200, 1000, 3500,
         {4.19864056, -2.0364341e-03, 6.52040211e-06, -5.48797062e-09, 1.77197817e-12, -3.02937267e+04, -0.849032208},
         {3.03399249, 2.17691804e-03, -1.64072518e-07, -9.7041987e-11, 1.68200992e-14, -3.00042971e+04, 4.9667701}),
      sp("HO2", {{"H", 1}, {"O", 2}}, 200, 1000, 3500,
         {4.30179801, -4.74912051e-03, 2.11582891e-05, -2.42763894e-08, 9.29225124e-12, 294.80804, 3.71666245},
         {4.0172109, 2.23982013e-03, -6.3365815e-07, 1.1424637e-10, -1.07908535e-14, 111.856713, 3.78510215}),
      sp("H2O2", {{"H", 2}, {"O", 2}}, 200, 1000, 3500,
         {4.27611269, -5.42822417e-04, 1.67335701e-05, -2.15770813e-08, 8.62454363e-12, -1.77025821e+04, 3.43505074},
         {4.16500285, 4.90831694e-03, -1.90139225e-06, 3.71185986e-10, -2.87908305e-14, -1.78617877e+04, 2.91615662}),
      sp("AR", {{"Ar", 1}}, 300, 1000, 5000, {2.5, 0, 0, 0, 0, -745.375, 4.366},
         {2.5, 0, 0, 0, 0, -745.375, 4.366}),
      sp("N2", {{"N", 2}}, 300, 1000, 5000,
         {3.298677, 1.4082404e-03, -3.963222e-06, 5.641515e-09, -2.444854e-12, -1020.8999, 3.950372},
         {2.92664, 1.4879768e-03, -5.68476e-07, 1.0097038e-10, -6.753351e-15, -922.7977, 5.980528}),
  };
}

inline kin::ReactionSpec elementary(std::map<std::string, double> r, std::map<std::string, double> p,
                                    double a, double b, double ea) {
  kin::ReactionSpec x;
  x.reactants = std::move(r); x.products = std::move(p); x.a = a; x.b = b; x.ea = ea;
  return x;
}
inline kin::ReactionSpec three_body(std::map<std::string, double> r, std::map<std::string, double> p,
                                    double a, double b, double ea, std::map<std::string, double> eff) {
  auto x = elementary(std::move(r), std::move(p), a, b, ea);
  x.type = kin::Type::ThreeBody;
  x.efficiencies = std::move(eff);
  return x;
}

inline std::vector<kin::ReactionSpec> reactions() {
  std::vector<kin::ReactionSpec> r;
  r.push_back(three_body({{"O", 2}}, {{"O2", 1}}, 1.2e17, -1.0, 0.0, {{"H2", 2.4}, {"H2O", 15.4}, {"AR", 0.83}}));
  r.push_back(three_body({{"O", 1}, {"H", 1}}, {{"OH", 1}}, 5.0e17, -1.0, 0.0, {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}}));
  r.push_back(elementary({{"O", 1}, {"H2", 1}}, {{"H", 1}, {"OH", 1}}, 3.87e4, 2.7, 6260.0));
  r.push_back(elementary({{"O", 1}, {"HO2", 1}}, {{"OH", 1}, {"O2", 1}}, 2.0e13, 0.0, 0.0));
  r.push_back(elementary({{"O", 1}, {"H2O2", 1}}, {{"OH", 1}, {"HO2", 1}}, 9.63e6, 2.0, 4000.0));
  r.push_back(three_body({{"H", 1}, {"O2", 1}}, {{"HO2", 1}}, 2.8e18, -0.86, 0.0, {{"O2", 0.0}, {"H2O", 0.0}, {"N2", 0.0}, {"AR", 0.0}}));
  r.push_back(elementary({{"H", 1}, {"O2", 2}}, {{"HO2", 1}, {"O2", 1}}, 2.08e19, -1.24, 0.0));
  r.push_back(elementary({{"H", 1}, {"O2", 1}, {"H2O", 1}}, {{"HO2", 1}, {"H2O", 1}}, 1.126e19, -0.76, 0.0));
  r.push_back(elementary({{"H", 1}, {"O2", 1}, {"N2", 1}}, {{"HO2", 1}, {"N2", 1}}, 2.6e19, -1.24, 0.0));
  r.push_back(elementary({{"H", 1}, {"O2", 1}, {"AR", 1}}, {{"HO2", 1}, {"AR", 1}}, 7.0e17, -0.8, 0.0));
  r.push_back(elementary({{"H", 1}, {"O2", 1}}, {{"O", 1}, {"OH", 1}}, 2.65e16, -0.6707, 1.7041e4));
  r.push_back(three_body({{"H", 2}}, {{"H2", 1}}, 1.0e18, -1.0, 0.0, {{"H2", 0.0}, {"H2O", 0.0}, {"AR", 0.63}}));
  r.push_back(elementary({{"H", 2}, {"H2", 1}}, {{"H2", 2}}, 9.0e16, -0.6, 0.0));
  r.push_back(elementary({{"H", 2}, {"H2O", 1}}, {{"H2", 1}, {"H2O", 1}}, 6.0e19, -1.25, 0.0));
  r.push_back(three_body({{"H", 1}, {"OH", 1}}, {{"H2O", 1}}, 2.2e22, -2.0, 0.0, {{"H2", 0.73}, {"H2O", 3.65}, {"AR", 0.38}}));
  r.push_back(elementary({{"H", 1}, {"HO2", 1}}, {{"O", 1}, {"H2O", 1}}, 3.97e12, 0.0, 671.0));
  r.push_back(elementary({{"H", 1}, {"HO2", 1}}, {{"O2", 1}, {"H2", 1}}, 4.48e13, 0.0, 1068.0));
  r.push_back(elementary({{"H", 1}, {"HO2", 1}}, {{"OH", 2}}, 8.4e13, 0.0, 635.0));
  r.push_back(elementary({{"H", 1}, {"H2O2", 1}}, {{"HO2", 1}, {"H2", 1}}, 1.21e7, 2.0, 5200.0));
  r.push_back(elementary({{"H", 1}, {"H2O2", 1}}, {{"OH", 1}, {"H2O", 1}}, 1.0e13, 0.0, 3600.0));
  r.push_back(elementary({{"OH", 1}, {"H2", 1}}, {{"H", 1}, {"H2O", 1}}, 2.16e8, 1.51, 3430.0));
  {
    kin::ReactionSpec f;
    f.reactants = {{"OH", 2}}; f.products = {{"H2O2", 1}};
    f.type = kin::Type::Falloff;
    f.a_low = 2.3e18; f.b_low = -0.9; f.ea_low = -1700.0;
    f.a = 7.4e13; f.b = -0.37; f.ea = 0.0;
    f.troe = true; f.troe_params = {0.7346, 94.0, 1756.0, 5182.0};
    f.efficiencies = {{"H2", 2.0}, {"H2O", 6.0}, {"AR", 0.7}};
    r.push_back(f);
  }
  r.push_back(elementary({{"OH", 2}}, {{"O", 1}, {"H2O", 1}}, 3.57e4, 2.4, -2110.0));
  r.push_back(elementary({{"OH", 1}, {"HO2", 1}}, {{"O2", 1}, {"H2O", 1}}, 1.45e13, 0.0, -500.0));
  r.push_back(elementary({{"OH", 1}, {"H2O2", 1}}, {{"HO2", 1}, {"H2O", 1}}, 2.0e12, 0.0, 427.0));
  r.push_back(elementary({{"OH", 1}, {"H2O2", 1}}, {{"HO2", 1}, {"H2O", 1}}, 1.7e18, 0.0, 2.941e4));
  r.push_back(elementary({{"HO2", 2}}, {{"O2", 1}, {"H2O2", 1}}, 1.3e11, 0.0, -1630.0));
  r.push_back(elementary({{"HO2", 2}}, {{"O2", 1}, {"H2O2", 1}}, 4.2e14, 0.0, 1.2e4));
  r.push_back(elementary({{"OH", 1}, {"HO2", 1}}, {{"O2", 1}, {"H2O", 1}}, 5.0e15, 0.0, 1.733e4));
  return r;
}

} // namespace h2o2
