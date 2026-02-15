#!/usr/bin/env python3
"""Generate Torch-aligned Series method surfaces from Torch native_functions.yaml.

Usage:
  python3 tools/codegen/generate_series_methods.py \
    --yaml <path-to-native_functions.yaml> \
    --out-methods src/CodeM/core/SeriesTorchAlignedMethods.inc \
    --out-stubs   src/CodeM/core/SeriesTorchAlignedStubs.inc

Notes:
- CPU-only subset: keeps ops whose first arg is `Tensor self`
- drops clearly non-CPU/autograd/backward-related names
- maps Torch `Tensor` -> `Series`, `Scalar/float` -> `double`, `int` -> `std::int64_t`
- skips device/layout/memory_format/generator/optional args (can be extended later)
"""
import argparse, re
from pathlib import Path

BAN_SUB = ["cuda","hip","xpu","vulkan","mps","metal","mkldnn","quantized","_backward","backward","grad","autograd","_fw","_cuda","_hip"]

def map_arg(arg: str):
  arg = arg.strip()
  if not arg or arg == "*" : return None
  arg = re.sub(r"Tensor\([^\)]*\)", "Tensor", arg)
  m = re.match(r"([A-Za-z0-9_?\[\]]+)\s+([A-Za-z0-9_]+)(=.*)?$", arg)
  if not m: return None
  ty, name, default = m.group(1), m.group(2), m.group(3) or ""
  ban_types = ["Device","Layout","MemoryFormat","Stream","Dimname","Generator","QScheme","ScalarType?","TensorOptions","DispatchKey","Storage","Sym"]
  if any(bt in ty for bt in ban_types): return None

  if ty == "Tensor":
    cty = f"const Series& {name}"
  elif ty == "Tensor[]":
    cty = f"std::span<const Series> {name}"
  elif ty == "int":
    cty = f"std::int64_t {name}"
  elif ty == "int[]":
    cty = f"std::span<const std::int64_t> {name}"
  elif ty == "bool":
    cty = f"bool {name}"
  elif ty in ["float","Scalar"]:
    cty = f"double {name}"
  elif ty == "ScalarType":
    cty = f"ScalarType {name}"
  else:
    return None

  if default:
    dv = default.lstrip("=").replace("None","0")
    if ty in ["bool","int","float","Scalar"]:
      cty += "=" + dv
  return cty

def sanitize_name(n: str):
  if n == "cat": return "unite"
  if n == "stack": return "pile"
  return n

def strip_defaults(sig: str) -> str:
  """Normalize a C++ parameter list string by removing default initializers.

  This lets us de-duplicate declarations that differ only by default arguments,
  which are NOT part of the C++ function signature.
  """
  # Remove any `=...` default initializer within each parameter
  parts = [p.strip() for p in sig.split(",") if p.strip()]
  nd = []
  for p in parts:
    nd.append(re.sub(r"\s*=\s*[^,]+$", "", p).strip())
  return ", ".join(nd)

def main():
  ap = argparse.ArgumentParser()
  ap.add_argument("--yaml", required=True)
  ap.add_argument("--out-methods", required=True)
  ap.add_argument("--out-stubs", required=True)
  a = ap.parse_args()

  text = Path(a.yaml).read_text(encoding="utf-8", errors="ignore")
  funcs = re.findall(r"^\s*-\s*func:\s*([^\n#]+)$", text, flags=re.MULTILINE)

  self_ops = []
  for f in funcs:
    m = re.match(r"([A-Za-z0-9_\.]+)\((.*)\)\s*->", f)
    if not m: continue
    name = m.group(1)
    argv = m.group(2).strip()
    first = argv.split(",")[0].strip() if argv else ""
    if first.startswith("Tensor self") or first.startswith("Tensor(a!) self") or first.startswith("Tensor(a) self"):
      lname = name.lower()
      if any(b in lname for b in BAN_SUB): continue
      self_ops.append((name, argv))

  groups = {}  # name -> { normalized_sig : emitted_sig }
  for full, argv in self_ops:
    base = full.split(".")[0]
    parts = [x.strip() for x in argv.split(",")][1:]  # drop self
    mapped = []
    ok = True
    for p in parts:
      if p == "*": continue
      ma = map_arg(p)
      if ma is None:
        ok = False; break
      mapped.append(ma)
    if not ok: continue
    sig = ", ".join(mapped)
    nsig = strip_defaults(sig)
    groups.setdefault(base, {})
    # Prefer the signature that carries defaults (more user-friendly) if present
    prev = groups[base].get(nsig)
    if prev is None or ("=" not in prev and "=" in sig):
      groups[base][nsig] = sig

  # augment with scalar rhs when only 1 Series arg
  aug = {k: dict(v) for k, v in groups.items()}  # copy
  for name, sigmap in groups.items():
    for nsig, sig in list(sigmap.items()):
      if sig.count("const Series&") == 1:
        pname = re.findall(r"const Series&\s+([A-Za-z0-9_]+)", sig)[0]
        aug_sig = re.sub(rf"const Series&\s+{pname}", f"double {pname}", sig)
        aug_nsig = strip_defaults(aug_sig)
        prev = aug[name].get(aug_nsig)
        if prev is None or ("=" not in prev and "=" in aug_sig):
          aug[name][aug_nsig] = aug_sig

  decls, stubs = [], []
  for name in sorted(aug.keys()):
    cpp = sanitize_name(name)
    for sig in sorted(aug[name].values()):
      decls.append(f"  Series {cpp}({sig}) const;")
      decls.append(f"  Series& {cpp}_({sig});")
      stubs.append(f"Series Series::{cpp}({sig}) const {{ PLUSS_CALL_GUARD(); throw_error(\"{cpp}: not implemented yet\"); }}")
      stubs.append(f"Series& Series::{cpp}_({sig}) {{ PLUSS_CALL_GUARD(); throw_error(\"{cpp}_: not implemented yet\"); }}")

  Path(a.out_methods).write_text("\n".join(decls) + "\n", encoding="utf-8")
  Path(a.out_stubs).write_text("\n".join(stubs) + "\n", encoding="utf-8")

if __name__ == "__main__":
  main()
