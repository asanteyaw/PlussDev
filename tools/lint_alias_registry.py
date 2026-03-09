#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REGISTRY_PATH = ROOT / "docs" / "alias_registry.yml"

ALIAS_RE = re.compile(r"^[a-z][a-z0-9]{0,3}$")


def _error(message: str, errors: list[str]) -> None:
  errors.append(message)


def _load_registry(errors: list[str]) -> dict:
  if not REGISTRY_PATH.exists():
    _error(f"Missing registry file: {REGISTRY_PATH}", errors)
    return {}
  try:
    return json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))
  except Exception as exc:  # noqa: BLE001
    _error(f"Failed to parse {REGISTRY_PATH}: {exc}", errors)
    return {}


def _validate_registry_shape(registry: dict, errors: list[str]) -> list[dict]:
  if not isinstance(registry, dict):
    _error("Registry root must be an object", errors)
    return []
  aliases = registry.get("aliases")
  if not isinstance(aliases, list):
    _error("Registry must contain an 'aliases' list", errors)
    return []
  return [entry for entry in aliases if isinstance(entry, dict)]


def _validate_entries(entries: list[dict], errors: list[str]) -> tuple[dict, dict]:
  by_scope_alias: dict[tuple[str, str], dict] = {}
  by_file_alias: dict[tuple[str, str], dict] = {}
  required = {"scope", "alias", "target", "file", "values"}

  for idx, entry in enumerate(entries):
    missing = sorted(required.difference(entry.keys()))
    if missing:
      _error(f"aliases[{idx}] missing required keys: {', '.join(missing)}", errors)
      continue

    scope = entry["scope"]
    alias = entry["alias"]
    target = entry["target"]
    file_rel = entry["file"]
    values = entry["values"]

    if not isinstance(scope, str) or not scope:
      _error(f"aliases[{idx}].scope must be a non-empty string", errors)
      continue
    if not isinstance(alias, str) or not ALIAS_RE.fullmatch(alias):
      _error(
          f"aliases[{idx}].alias='{alias}' must match lowercase <=4 char policy",
          errors,
      )
      continue
    if not isinstance(target, str) or not target:
      _error(f"aliases[{idx}].target must be a non-empty string", errors)
      continue
    if not isinstance(file_rel, str) or not file_rel:
      _error(f"aliases[{idx}].file must be a non-empty string", errors)
      continue
    if not isinstance(values, list) or not values or not all(isinstance(v, str) for v in values):
      _error(f"aliases[{idx}].values must be a non-empty string list", errors)
      continue
    if len(set(values)) != len(values):
      _error(f"aliases[{idx}].values contains duplicates", errors)
      continue

    if (scope, alias) in by_scope_alias:
      prev = by_scope_alias[(scope, alias)]
      _error(
          "Collision: "
          f"scope '{scope}' reuses alias '{alias}' "
          f"for both '{prev['target']}' and '{target}'",
          errors,
      )
      continue

    if (file_rel, alias) in by_file_alias:
      prev = by_file_alias[(file_rel, alias)]
      _error(
          "Collision: "
          f"file '{file_rel}' reuses alias namespace '{alias}' "
          f"for both '{prev['target']}' and '{target}'",
          errors,
      )
      continue

    file_abs = ROOT / file_rel
    if not file_abs.exists():
      _error(f"aliases[{idx}] references missing file: {file_rel}", errors)
      continue

    by_scope_alias[(scope, alias)] = entry
    by_file_alias[(file_rel, alias)] = entry

  return by_scope_alias, by_file_alias


def _validate_alias_block(file_text: str, file_rel: str, entry: dict, errors: list[str]) -> None:
  alias = entry["alias"]
  target = entry["target"]
  expected_values = entry["values"]

  block_pat = re.compile(
      rf"namespace\s+{re.escape(alias)}\s*\{{(?P<body>.*?)\}}\s*//\s*namespace\s+{re.escape(alias)}",
      re.S,
  )
  block = block_pat.search(file_text)
  if not block:
    _error(f"{file_rel}: missing alias namespace '{alias}'", errors)
    return

  body = block.group("body")
  const_pat = re.compile(
      rf"inline\s+constexpr\s+{re.escape(target)}\s+([A-Za-z_]\w*)\s*=\s*{re.escape(target)}::([A-Za-z_]\w*)\s*;"
  )
  constants = const_pat.findall(body)
  if not constants:
    _error(f"{file_rel}: namespace '{alias}' has no '{target}' constexpr aliases", errors)
    return

  for lhs, rhs in constants:
    if lhs != rhs:
      _error(
          f"{file_rel}: namespace '{alias}' has non-identity alias '{lhs} = {target}::{rhs}'",
          errors,
      )

  found_values = [lhs for lhs, _ in constants]
  if found_values != expected_values:
    expected_set = set(expected_values)
    found_set = set(found_values)
    missing = sorted(expected_set.difference(found_set))
    extra = sorted(found_set.difference(expected_set))
    _error(
        f"{file_rel}: namespace '{alias}' values mismatch "
        f"(missing={missing}, extra={extra}, expected_order={expected_values}, found_order={found_values})",
        errors,
    )


def _scan_unregistered_aliases(registered: dict, errors: list[str]) -> None:
  include_roots = [
      ROOT / module / "include"
      for module in ("PlussAI", "PlussMa", "PlussMic", "PlussPl", "PlussQit", "PlussView")
      if (ROOT / module / "include").exists()
  ]
  candidate_files = [
      path
      for root in include_roots
      for path in list(root.rglob("*.h")) + list(root.rglob("*.hpp"))
  ]

  ns_block = re.compile(
      r"namespace\s+([a-z][a-z0-9]{0,3})\s*\{(?P<body>.*?)\}\s*//\s*namespace\s+\1",
      re.S,
  )
  enum_alias_line = re.compile(
      r"inline\s+constexpr\s+([A-Za-z_][\w:]*)\s+([A-Za-z_]\w*)\s*=\s*\1::([A-Za-z_]\w*)\s*;"
  )

  for path in candidate_files:
    try:
      text = path.read_text(encoding="utf-8")
    except Exception:  # noqa: BLE001
      continue
    rel = str(path.relative_to(ROOT))
    for match in ns_block.finditer(text):
      alias = match.group(1)
      body = match.group("body")
      if not enum_alias_line.search(body):
        continue
      if (rel, alias) not in registered:
        _error(
            f"{rel}: alias namespace '{alias}' appears unregistered in docs/alias_registry.yml",
            errors,
        )


def main() -> int:
  errors: list[str] = []
  registry = _load_registry(errors)
  entries = _validate_registry_shape(registry, errors)
  _, by_file_alias = _validate_entries(entries, errors)

  for (file_rel, _alias), entry in by_file_alias.items():
    file_abs = ROOT / file_rel
    try:
      file_text = file_abs.read_text(encoding="utf-8")
    except Exception as exc:  # noqa: BLE001
      _error(f"Failed to read {file_rel}: {exc}", errors)
      continue
    _validate_alias_block(file_text, file_rel, entry, errors)

  _scan_unregistered_aliases(by_file_alias, errors)

  if errors:
    print("alias_registry lint FAILED")
    for message in errors:
      print(f"  - {message}")
    return 1

  print(
      "alias_registry lint PASSED "
      f"({len(by_file_alias)} registered alias namespaces checked)"
  )
  return 0


if __name__ == "__main__":
  sys.exit(main())
