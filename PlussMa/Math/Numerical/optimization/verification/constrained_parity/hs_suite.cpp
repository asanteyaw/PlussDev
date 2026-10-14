#include <iostream>
#include <pluss/ma.h>
#include <iomanip>
namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
auto f64 = pluss::ScalarType::Float64;
pluss::Series sc(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {(int64_t)v.size()}).to(f64); }
optim::ParameterPack<pluss::Series> pk(std::vector<std::pair<std::string,double>> b) {
  std::vector<optim::ParameterPack<pluss::Series>::Block> out;
  for (auto& [n,v]: b) out.push_back({n, pluss::make_series(v).to(f64).reshape({1})});
  return optim::ParameterPack<pluss::Series>(out);
}
void report(const char* name, optim::OptimaAlgorithm a, const optim::OptimaState& r, double fstar) {
  std::cout << std::setw(8) << name << " " << std::setw(17) << (a==optim::OptimaAlgorithm::SequentialQP?"SQP":a==optim::OptimaAlgorithm::InternalProtocol?"IP":a==optim::OptimaAlgorithm::WorkingSet?"WS":"MERIT")
            << " flag=" << std::setw(2) << (int)r.exit_flag << " it=" << std::setw(3) << r.iteration << " ev=" << std::setw(4) << r.function_evaluations
            << " f=" << std::setprecision(8) << r.evaluation.value.item<double>() << " (f*=" << fstar << ") viol=" << std::scientific << std::setprecision(2) << r.evaluation.max_constraint_violation << std::fixed << "\n";
}
int main() {
  for (auto alg : {optim::OptimaAlgorithm::MeritMethod}) {
    optim::OptimaOptions o; o.algorithm = alg;
    { // HS71
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { auto x1=q.at("x1"),x2=q.at("x2"),x3=q.at("x3"),x4=q.at("x4"); return x1*x4*(x1+x2+x3)+x3; };
      p.nonlinear.inequality = [](const Pack& q) { auto x1=q.at("x1"),x2=q.at("x2"),x3=q.at("x3"),x4=q.at("x4"); return 25.0 - x1*x2*x3*x4; };
      p.nonlinear.equality = [](const Pack& q) { return optim::square(q.at("x1"))+optim::square(q.at("x2"))+optim::square(q.at("x3"))+optim::square(q.at("x4")) - 40.0; };
      p.bounds.lower = pk({{"x1",1},{"x2",1},{"x3",1},{"x4",1}}); p.bounds.upper = pk({{"x1",5},{"x2",5},{"x3",5},{"x4",5}});
      report("HS71", alg, optim::optima(p, pk({{"x1",1},{"x2",5},{"x3",5},{"x4",1}}), o), 17.0140173);
    }
    { // HS35
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { auto x1=q.at("x1"),x2=q.at("x2"),x3=q.at("x3"); return 9.0 - 8.0*x1 - 6.0*x2 - 4.0*x3 + 2.0*x1*x1 + 2.0*x2*x2 + x3*x3 + 2.0*x1*x2 + 2.0*x1*x3; };
      p.nonlinear.inequality = [](const Pack& q) { return q.at("x1")+q.at("x2")+2.0*q.at("x3") - 3.0; };
      p.bounds.lower = pk({{"x1",0},{"x2",0},{"x3",0}});
      report("HS35", alg, optim::optima(p, pk({{"x1",0.5},{"x2",0.5},{"x3",0.5}}), o), 1.0/9.0);
    }
    { // HS21
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { return 0.01*optim::square(q.at("x1")) + optim::square(q.at("x2")) - 100.0; };
      p.nonlinear.inequality = [](const Pack& q) { return 10.0 - 10.0*q.at("x1") + q.at("x2"); };
      p.bounds.lower = pk({{"x1",2},{"x2",-50}}); p.bounds.upper = pk({{"x1",50},{"x2",50}});
      report("HS21", alg, optim::optima(p, pk({{"x1",-1},{"x2",-1}}), o), -99.96);
    }
    { // HS6 equality: min (1-x1)^2  s.t. 10(x2-x1^2)=0, start (-1.2,1)
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { return optim::square(1.0 - q.at("x1")); };
      p.nonlinear.equality = [](const Pack& q) { return 10.0*(q.at("x2") - optim::square(q.at("x1"))); };
      report("HS6", alg, optim::optima(p, pk({{"x1",-1.2},{"x2",1}}), o), 0.0);
    }
    { // HS12: min 0.5x1^2 + x2^2 - x1x2 - 7x1 - 7x2 s.t. 25 - 4x1^2 - x2^2 >= 0, start (0,0): f*=-30, x*=(2,3)
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { auto x1=q.at("x1"), x2=q.at("x2"); return 0.5*x1*x1 + x2*x2 - x1*x2 - 7.0*x1 - 7.0*x2; };
      p.nonlinear.inequality = [](const Pack& q) { return 4.0*optim::square(q.at("x1")) + optim::square(q.at("x2")) - 25.0; };
      report("HS12", alg, optim::optima(p, pk({{"x1",0},{"x2",0}}), o), -30.0);
    }
    { // HS26-like equality with 3 vars: min (x1-x2)^2+(x2-x3)^4 s.t. x1(1+x2^2)+x3^4=3, start (-2.6,2,2): f*=0
      optim::ConstrainedProblem p;
      p.objective.autodiff = [](const Pack& q) { return optim::square(q.at("x1")-q.at("x2")) + optim::pow(q.at("x2")-q.at("x3"), 4.0); };
      p.nonlinear.equality = [](const Pack& q) { return q.at("x1")*(1.0+optim::square(q.at("x2"))) + optim::pow(q.at("x3"),4.0) - 3.0; };
      report("HS26", alg, optim::optima(p, pk({{"x1",-2.6},{"x2",2},{"x3",2}}), o), 0.0);
    }
  }
}
