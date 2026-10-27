#!/usr/bin/env python3
"""Emits suite_matlab.m and suite_ours.cpp from problems.py so both solvers see identical problems."""
import ast, math, sys
from pathlib import Path
from problems import P, INF

FUN = {"exp", "log", "sqrt", "sin", "cos"}

def has_x(n):
    return any(isinstance(m, ast.Name) and m.id.startswith("x") for m in ast.walk(n))

def num(v):
    return repr(float(v))

def emit(n, mode):
    """mode 'm' = MATLAB, 'c' = C++ (Trace)."""
    if not has_x(n):
        val = eval(compile(ast.Expression(n), "", "eval"), {"__builtins__": {}}, {"pi": math.pi, "sqrt": math.sqrt, "sin": math.sin, "cos": math.cos, "exp": math.exp, "log": math.log})
        return num(val)
    if isinstance(n, ast.Name):
        i = int(n.id[1:])
        return f"x({i})" if mode == "m" else f"X(q,{i})"
    if isinstance(n, ast.UnaryOp):
        return f"(-{emit(n.operand, mode)})"
    if isinstance(n, ast.Call):
        f = n.func.id
        return f"{f}({emit(n.args[0], mode)})" if mode == "m" else f"optim::{f}({emit(n.args[0], mode)})"
    if isinstance(n, ast.BinOp):
        a, b = emit(n.left, mode), emit(n.right, mode)
        if isinstance(n.op, ast.Pow):
            if mode == "m":
                return f"({a}^{b})"
            if isinstance(n.right, ast.Constant) and n.right.value == 2:
                return f"optim::square({a})"
            return f"optim::pow({a}, {b})"
        op = {ast.Add: "+", ast.Sub: "-", ast.Mult: "*", ast.Div: "/"}[type(n.op)]
        return f"({a} {op} {b})"
    raise ValueError(ast.dump(n))

def expr(s, mode):
    return emit(ast.parse(s, mode="eval").body, mode)

def mat_vec(v):
    return "[" + " ".join("Inf" if x == INF else "-Inf" if x == -INF else repr(float(x)) for x in v) + "]"

def gen_matlab(path):
    L = ["function suite_matlab(outfile)", "fid = fopen(outfile,'w');", "algs = {'interior-point','sqp','active-set'};", ""]
    for p in P:
        n = len(p["x0"])
        L.append(f"% {p['name']}")
        L.append(f"P.name='{p['name']}'; P.x0={mat_vec(p['x0'])}'; P.lb={mat_vec(p['lb']) if p['lb'] else '[]'}'; P.ub={mat_vec(p['ub']) if p['ub'] else '[]'}';")
        L.append(f"P.f=@(x) {expr(p['f'], 'm')};")
        c = ";".join(expr(g, "m") for g in p["ineq"])
        e = ";".join(expr(h, "m") for h in p["eq"])
        if p["ineq"] or p["eq"]:
            L.append(f"P.nl=@(x) deal([{c}],[{e}]);")
        else:
            L.append("P.nl=[];")
        L.append("run_one(fid,P,algs);")
        L.append("")
    L.append("fclose(fid);\nend\n")
    L.append("""function run_one(fid,P,algs)
for a = 1:numel(algs)
  o = optimoptions('fmincon','Algorithm',algs{a},'Display','off','MaxIterations',1000,'MaxFunctionEvaluations',50000);
  try
    [~,f,flag,out] = fmincon(P.f,P.x0,[],[],[],[],P.lb,P.ub,P.nl,o);
    fprintf(fid,'%s|%s|%d|%d|%d|%.12g|%.3e\\n',P.name,algs{a},flag,out.iterations,out.funcCount,f,out.constrviolation);
  catch err
    msg = strrep(strtok(err.message, newline),'|','/');
    fprintf(fid,'%s|%s|ERR|0|0|NaN|NaN|%s\\n',P.name,algs{a},msg(1:min(60,end)));
  end
end
end""")
    Path(path).write_text("\n".join(L))

def cvec(v):
    return "{" + ", ".join("-INF" if x == -INF else "INF" if x == INF else repr(float(x)) for x in v) + "}"

def gen_cpp(path):
    L = ['''#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <pluss/ma.h>
#include <string>
#include <vector>
namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<optim::Trace>;
using Series = pluss::Series;
static const auto f64 = pluss::ScalarType::Float64;
static const double INF = std::numeric_limits<double>::infinity();
static Series pk1(const std::vector<double>& v, const std::string& n) { return pluss::make_series(v, {(int64_t)v.size()}).to(f64); }
static optim::ParameterPack<Series> pk(const std::vector<double>& v) {
  std::vector<optim::ParameterPack<Series>::Block> out;
  for (size_t i = 0; i < v.size(); ++i) out.push_back({"x" + std::to_string(i + 1), pluss::make_series(v[i]).to(f64).reshape({1})});
  return optim::ParameterPack<Series>(out);
}
static optim::Trace X(const Pack& q, int i) { return q.at("x" + std::to_string(i)); }
struct Prob { std::string name; std::vector<double> x0; optim::ConstrainedProblem p; };
static std::vector<Prob> problems() {
  std::vector<Prob> P;
''']
    for p in P:
        L.append("  { Prob s{\"%s\", %s, {}};" % (p["name"], cvec(p["x0"])))
        L.append(f"    s.p.objective.autodiff = [](const Pack& q) {{ return {expr(p['f'], 'c')}; }};")
        if p["lb"]: L.append(f"    s.p.bounds.lower = pk({cvec(p['lb'])});")
        if p["ub"]: L.append(f"    s.p.bounds.upper = pk({cvec(p['ub'])});")
        def vec(lst):
            items = ", ".join(expr(g, "c") for g in lst)
            return items
        for key, field in (("ineq", "inequality"), ("eq", "equality")):
            lst = p[key]
            if not lst: continue
            if len(lst) == 1:
                L.append(f"    s.p.nonlinear.{field} = [](const Pack& q) {{ return {expr(lst[0], 'c')}; }};")
            else:
                L.append(f"    s.p.nonlinear.{field} = [](const Pack& q) {{ return pluss::subsp::math::diff::unite(std::vector<optim::Trace>{{{vec(lst)}}}, 0); }};")
        L.append("    P.push_back(s); }")
    L.append("  return P;\n}")
    L.append('''
int main() {
  const optim::OptimaAlgorithm algs[4] = {optim::OptimaAlgorithm::InternalProtocol, optim::OptimaAlgorithm::SequentialQP,
                                          optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::MeritMethod};
  const char* names[4] = {"interior-point", "sqp", "active-set", "merit"};
  for (auto& pr : problems()) {
    for (int a = 0; a < 4; ++a) {
      optim::OptimaOptions o; o.algorithm = algs[a];
      o.sequential_qp.max_iterations = 1000; o.working_set.max_iterations = 1000;
      o.internal_protocol.max_iterations = 1000; o.penalty.max_iterations = 1000;
      try {
        auto r = optim::optima(pr.p, pk(pr.x0), o);
        std::string m = r.message; m = m.substr(0, m.find('\\n')); m = m.substr(0, 60);
        std::cout << pr.name << "|" << names[a] << "|" << (int)r.exit_flag << "|" << r.iteration << "|" << r.function_evaluations << "|"
                  << std::setprecision(12) << r.evaluation.value.item<double>() << "|" << std::scientific << std::setprecision(3)
                  << r.evaluation.max_constraint_violation << std::defaultfloat << "|" << m << "\\n";
      } catch (const std::exception& e) {
        std::string m = e.what(); m = m.substr(0, m.find('\\n'));
        std::cout << pr.name << "|" << names[a] << "|ERR|0|0|NaN|NaN|" << m.substr(0, 60) << "\\n";
      }
    }
  }
}''')
    Path(path).write_text("\n".join(L))

if __name__ == "__main__":
    out = Path(sys.argv[1] if len(sys.argv) > 1 else "/tmp/g/broad")
    out.mkdir(parents=True, exist_ok=True)
    gen_matlab(out / "suite_matlab.m")
    gen_cpp(out / "suite_ours.cpp")
    print("wrote", out)
