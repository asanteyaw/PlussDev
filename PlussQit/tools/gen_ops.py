#!/usr/bin/env python3
import argparse
import pathlib
import textwrap
import re

def write(path: pathlib.Path, s: str):
  path.parent.mkdir(parents=True, exist_ok=True)
  path.write_text(textwrap.dedent(s).lstrip())

def parse_yaml_minimal(yaml_text: str):
  lines = [ln.rstrip() for ln in yaml_text.splitlines() if ln.strip() and not ln.strip().startswith("#")]
  ops = []
  cur = None
  for ln in lines:
    if ln.startswith("ops:"):
      continue
    if ln.strip().startswith("- "):
      if cur:
        ops.append(cur)
      cur = {}
      rest = ln.strip()[2:]
      if rest.startswith("name:"):
        cur["name"] = rest.split(":",1)[1].strip()
    else:
      if cur is None:
        continue
      key, val = ln.strip().split(":", 1)
      val = val.strip()
      if val.startswith('"') and val.endswith('"'):
        val = val[1:-1]
      cur[key] = val
  if cur:
    ops.append(cur)
  return ops

def signature_parts(sig: str):
  m = re.match(r"\s*(\w+)\s+(\w+)\s*\((.*)\)\s*", sig)
  if not m:
    raise ValueError(f"Bad signature: {sig}")
  ret, name, args = m.group(1), m.group(2), m.group(3).strip()
  return ret, name, args

def gen(inp: pathlib.Path, out_dir: pathlib.Path):
  ops = parse_yaml_minimal(inp.read_text())
  out_ops_dir = out_dir / "CodeM" / "ops"
  out_ops_dir.mkdir(parents=True, exist_ok=True)

  for op in ops:
    sig = op["signature"]
    _, name, args = signature_parts(sig)
    params = [a.strip() for a in args.split(",")] if args else []
    arg_names = [p.split()[-1] for p in params if p]
    forward = ", ".join(arg_names)

    header = f"""#pragma once
#include <cstdint>
#include <vector>
#include "CodeM/core/Series.h"

namespace pluss::codem::{name}_ops {{
  {sig};
}}

namespace pluss::codem::ops {{
  inline {sig} {{
    return pluss::codem::{name}_ops::{name}({forward});
  }}
}}
"""
    write(out_ops_dir / f"{name}_ops.h", header)

  cpp = [
    "#include <cstdint>",
    "#include <vector>",
    "#include \"CodeM/core/Errors.h\"",
    "#include \"CodeM/core/Series.h\"",
    "#include \"CodeM/core/SeriesIterator.h\"",
    "#include \"CodeM/core/TypePromotion.h\"",
    "",
    "namespace pluss::codem::native::cpu {",
    "  // Elementwise kernels (out-parameter)",
    "  void add(const Series& a, const Series& b, const Series& out);",
    "  void sub(const Series& a, const Series& b, const Series& out);",
    "  void mul(const Series& a, const Series& b, const Series& out);",
    "  void div(const Series& a, const Series& b, const Series& out);",
    "  void neg(const Series& a, Series& out);",
    "  void abs(const Series& a, Series& out);",
    "  void exp(const Series& a, Series& out);",
    "  void log(const Series& a, Series& out);",
    "  void sqrt(const Series& a, Series& out);",
    "  void sin(const Series& a, Series& out);",
    "  void cos(const Series& a, Series& out);",
    "  void tanh(const Series& a, Series& out);",
    "  void eq(const Series& a, const Series& b, const Series& out);",
    "  void ne(const Series& a, const Series& b, const Series& out);",
    "  void lt(const Series& a, const Series& b, const Series& out);",
    "  void le(const Series& a, const Series& b, const Series& out);",
    "  void gt(const Series& a, const Series& b, const Series& out);",
    "  void ge(const Series& a, const Series& b, const Series& out);",
    "  void minimum(const Series& a, const Series& b, const Series& out);",
    "  void maximum(const Series& a, const Series& b, const Series& out);",
    "  void clamp(const Series& a, Series& out, double lo, double hi);",
    "",
    "  // Return-by-value ops",
    "  Series sum(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series mean(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series amax(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series amin(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series argmax(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series argmin(const Series& a, std::int64_t dim, bool keepdim);",
    "  Series index_select(const Series& a, std::int64_t dim, const Series& index);",
    "  Series masked_select(const Series& a, const Series& mask);",
    "  Series matmul(const Series& a, const Series& b);",
    "  Series gather(const Series& input, std::int64_t dim, const Series& index);",
    "  Series scatter(const Series& input, std::int64_t dim, const Series& index, const Series& src);",
    "  Series scatter_add(const Series& input, std::int64_t dim, const Series& index, const Series& src);",
    "  Series where(const Series& cond, const Series& a, const Series& b);",
    "  Series masked_fill(const Series& a, const Series& mask, double value);",
    "  Series nonzero(const Series& mask);",
    "  Series take(const Series& a, const Series& index);",
    "  Series put(const Series& a, const Series& index, const Series& src, bool accumulate);",
    "  Series index(const Series& a, const std::vector<Series>& indices);",
    "  Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate);",
    "  Series cat(const std::vector<Series>& tensors, std::int64_t dim);",
    "  Series stack(const std::vector<Series>& tensors, std::int64_t dim);",
    "}",
    "",
    "namespace pluss::codem::ops_internal {",
    "",
  ]

  special = { "sum","mean","amax","amin","argmax","argmin","index_select","masked_select","matmul","gather","scatter",
              "scatter_add","where","masked_fill","nonzero","take","put","index","index_put","cat","stack",
              "sum_all","all_all","any_all" }

  cmp_ops = {"eq","ne","lt","le","gt","ge"}
  minmax_ops = {"minimum","maximum"}
  clamp_ops = {"clamp"}


  for op in ops:
    sig = op["signature"]
    _, name, args = signature_parts(sig)
    params = [a.strip() for a in args.split(",")] if args else []
    arg_names = [p.split()[-1] for p in params if p]
    forward = ", ".join(arg_names)

    cpp.append(f"namespace {name}_ops {{")
    cpp.append(f"{sig} {{")
    if name in special:
      cpp.append(f"  return pluss::codem::native::cpu::{name}({forward});")
    elif name in cmp_ops:
      cpp.append("  if (!a.defined() || !b.defined()) throw_error(\"op: undefined input\");")
      cpp.append("  // comparisons: bool output, numeric inputs must be same dtype after promotion")
      cpp.append("  const auto in_dtype = promote_types(a.scalar_type(), b.scalar_type());")
      cpp.append("  auto aa = (a.scalar_type() == in_dtype) ? a : a.to(in_dtype);")
      cpp.append("  auto bb = (b.scalar_type() == in_dtype) ? b : b.to(in_dtype);")
      cpp.append("  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());")
      cpp.append("  Series out = Series::empty(shape, ScalarType::Bool);")
      cpp.append(f"  pluss::codem::native::cpu::{name}(aa, bb, out);")
      cpp.append("  return out;")
    elif name in minmax_ops:
      cpp.append("  if (!a.defined() || !b.defined()) throw_error(\"op: undefined input\");")
      cpp.append("  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());")
      cpp.append("  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);")
      cpp.append("  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);")
      cpp.append("  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());")
      cpp.append("  Series out = Series::empty(shape, out_dtype);")
      cpp.append(f"  pluss::codem::native::cpu::{name}(aa, bb, out);")
      cpp.append("  return out;")
    elif name in clamp_ops:
      cpp.append("  if (!a.defined()) throw_error(\"op: undefined input\");")
      cpp.append("  Series out = Series::empty(a.sizes(), a.scalar_type());")
      cpp.append("  pluss::codem::native::cpu::clamp(a, out, lo, hi);")
      cpp.append("  return out;")
    elif len(arg_names) == 1:
      cpp.append("  if (!a.defined()) throw_error(\"op: undefined input\");")
      cpp.append("  Series out = Series::empty(a.sizes(), a.scalar_type());")
      cpp.append(f"  pluss::codem::native::cpu::{name}(a, out);")
      cpp.append("  return out;")
    else:
      cpp.append("  if (!a.defined() || !b.defined()) throw_error(\"op: undefined input\");")
      cpp.append("  const auto out_dtype = promote_types(a.scalar_type(), b.scalar_type());")
      cpp.append("  auto aa = (a.scalar_type() == out_dtype) ? a : a.to(out_dtype);")
      cpp.append("  auto bb = (b.scalar_type() == out_dtype) ? b : b.to(out_dtype);")
      cpp.append("  auto shape = SeriesIterator::broadcast_shape(aa.sizes(), bb.sizes());")
      cpp.append("  Series out = Series::empty(shape, out_dtype);")
      cpp.append(f"  pluss::codem::native::cpu::{name}(aa, bb, out);")
      cpp.append("  return out;")
    cpp.append("}")
    cpp.append("} // namespace")
    cpp.append("")
  cpp.append("} // namespace pluss::codem::ops_internal")
  write(out_dir / "CodeM" / "ops" / "ops_generated.cpp", "\n".join(cpp))

def main():
  ap = argparse.ArgumentParser()
  ap.add_argument("--in", dest="inp", required=True)
  ap.add_argument("--out", dest="out", required=True)
  args = ap.parse_args()
  gen(pathlib.Path(args.inp), pathlib.Path(args.out))

if __name__ == "__main__":
  main()
