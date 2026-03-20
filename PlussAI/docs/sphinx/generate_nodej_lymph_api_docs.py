#!/usr/bin/env python3
from __future__ import annotations

import re
import shutil
import os
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

ROOT = Path(__file__).resolve().parents[3]
DOCS_DIR = ROOT / "PlussAI" / "docs" / "sphinx"

NODEJ_HEADERS = sorted((ROOT / "PlussAI" / "include" / "pluss" / "ai" / "nodej").rglob("*.h"))
LYMPH_HEADERS = sorted((ROOT / "PlussAI" / "include" / "pluss" / "ai" / "lymph").rglob("*.h"))
EXAMPLE_FILES = sorted((ROOT / "PlussAI" / "examples" / "NodeJ").rglob("*.cpp"))
BUILD_EXE_ROOTS = [
    ROOT / "build" / "PlussAI" / "examples" / "NodeJ",
    ROOT / "build_cpp_env" / "PlussAI" / "examples" / "NodeJ",
]
RUN_LOG_DIR = DOCS_DIR / "generated" / "run_logs"


@dataclass
class ApiEntry:
    domain: str  # nodej | lymph
    kind: str  # method | function
    owner: Optional[str]
    name: str
    signature: str
    header: Path
    line: int
    is_static_method: bool = False


@dataclass
class TypeEntry:
    domain: str
    kind: str  # struct | enum | alias
    name: str
    header: Path
    line: int
    fields: List[str] = field(default_factory=list)
    alias_target: Optional[str] = None


@dataclass
class ParamInfo:
    name: str
    type_text: str
    default: Optional[str] = None


@dataclass
class SignatureInfo:
    return_type: str
    qualified_name: str
    params: List[ParamInfo]
    tail: str


def normalize_ws(text: str) -> str:
    return " ".join(text.replace("\t", " ").split())


def strip_comments(line: str) -> str:
    if "//" in line:
        return line.split("//", 1)[0]
    return line


def slugify(text: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", text.lower())
    return slug.strip("-") or "entry"


def rel_to_root(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_macro_invocation(line: str) -> bool:
    s = line.strip()
    return bool(re.match(r"^[A-Z][A-Z0-9_]*\s*\(.*\)\s*;?$", s))


def find_example_occurrences() -> Dict[str, List[Tuple[Path, int]]]:
    token_map: Dict[str, List[Tuple[Path, int]]] = {}
    token_rx = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
    for f in EXAMPLE_FILES:
        lines = f.read_text(encoding="utf-8", errors="ignore").splitlines()
        for i, line in enumerate(lines, start=1):
            for m in token_rx.finditer(line):
                token = m.group(1)
                token_map.setdefault(token, []).append((f, i))
    return token_map


def split_top_level(text: str, sep: str = ",") -> List[str]:
    parts: List[str] = []
    cur: List[str] = []
    a = p = b = c = 0
    in_single = False
    in_double = False
    esc = False
    for ch in text:
        if esc:
            cur.append(ch)
            esc = False
            continue
        if ch == "\\":
            cur.append(ch)
            esc = True
            continue
        if ch == "'" and not in_double:
            in_single = not in_single
            cur.append(ch)
            continue
        if ch == '"' and not in_single:
            in_double = not in_double
            cur.append(ch)
            continue
        if not in_single and not in_double:
            if ch == "<":
                a += 1
            elif ch == ">":
                a = max(0, a - 1)
            elif ch == "(":
                p += 1
            elif ch == ")":
                p = max(0, p - 1)
            elif ch == "[":
                b += 1
            elif ch == "]":
                b = max(0, b - 1)
            elif ch == "{":
                c += 1
            elif ch == "}":
                c = max(0, c - 1)
            elif ch == sep and a == 0 and p == 0 and b == 0 and c == 0:
                token = "".join(cur).strip()
                if token:
                    parts.append(token)
                cur = []
                continue
        cur.append(ch)
    tail = "".join(cur).strip()
    if tail:
        parts.append(tail)
    return parts


def split_default(param: str) -> Tuple[str, Optional[str]]:
    chunks = split_top_level(param, sep="=")
    if len(chunks) <= 1:
        return param.strip(), None
    left = chunks[0].strip()
    right = "=".join(chunks[1:]).strip()
    return left, right or None


def parse_single_param(param: str, idx: int) -> ParamInfo:
    p = param.strip()
    if p == "self":
        return ParamInfo(name="self", type_text="receiver")
    left, default = split_default(p)
    left = normalize_ws(left)
    name_match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*$", left)
    if name_match:
        name = name_match.group(1)
        type_text = left[: name_match.start()].strip() or "auto"
    else:
        name = f"arg{idx}"
        type_text = left or "auto"
    return ParamInfo(name=name, type_text=type_text, default=default)


def parse_signature_info(signature: str) -> SignatureInfo:
    s = signature.strip().rstrip(";")
    m = re.match(r"^(.*?)\s+([A-Za-z_][A-Za-z0-9_:]*)\((.*)\)\s*(.*)$", s)
    if not m:
        return SignatureInfo(return_type="auto", qualified_name=s, params=[], tail="")
    ret = normalize_ws(m.group(1))
    ret = re.sub(r"\[\[[^\]]+\]\]\s*", "", ret).strip()
    ret = re.sub(r"\b(inline|constexpr|virtual|static|friend|explicit)\b", "", ret).strip()
    ret = normalize_ws(ret)
    qualified_name = m.group(2).strip()
    args_text = m.group(3).strip()
    tail = normalize_ws(m.group(4).strip())
    params: List[ParamInfo] = []
    if args_text and args_text != "void":
        for idx, p in enumerate(split_top_level(args_text, sep=","), start=1):
            params.append(parse_single_param(p, idx))
    return SignatureInfo(return_type=ret, qualified_name=qualified_name, params=params, tail=tail)


def clean_type_for_docs(type_text: str) -> str:
    t = normalize_ws(type_text.strip())
    t = re.sub(r"\b(virtual|inline|constexpr|friend|static)\b", "", t)
    t = normalize_ws(t)
    return t


def public_signature_line(sig: SignatureInfo) -> str:
    params: List[str] = []
    for p in sig.params:
        if p.name == "self":
            continue
        ptype = clean_type_for_docs(p.type_text)
        if p.default is not None:
            params.append(f"{p.name}: {ptype} = {p.default}")
        else:
            params.append(f"{p.name}: {ptype}")
    ret = clean_type_for_docs(sig.return_type or "auto")
    return f"{sig.qualified_name}({', '.join(params)}) -> {ret}"


def sanitize_output_line(line: str) -> str:
    # Remove concrete internal/repo paths from public-facing docs.
    s = line
    s = re.sub(r"<repo_root>/\S+", "<path>", s)
    s = re.sub(r"\b(?:PlussAI|PlussQit|PlussMic|PlussMa|build_cpp_env|build)/\S+", "<path>", s)
    return s


def compute_snippet_window(path: Path, line: int, up: int = 8, down: int = 12) -> Tuple[int, int]:
    raw_lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    total = len(raw_lines)
    start = max(1, line - up)
    end = min(total, line + down)
    return start, end


def choose_examples(
    token_map: Dict[str, List[Tuple[Path, int]]],
    domain: str,
    name: str,
    max_examples: int = 3,
    for_type: bool = False,
) -> List[Tuple[Path, int, int]]:
    hits = token_map.get(name, [])
    if not hits:
        if for_type:
            # Prefer concrete object declarations for type pages.
            word_rx = re.compile(rf"\b{re.escape(name)}\b\s+[A-Za-z_][A-Za-z0-9_]*\s*[\{{=(]")
        else:
            word_rx = re.compile(rf"\b{re.escape(name)}\b")
        for f in EXAMPLE_FILES:
            lines = f.read_text(encoding="utf-8", errors="ignore").splitlines()
            for i, line in enumerate(lines, start=1):
                if word_rx.search(line):
                    hits.append((f, i))
    if not hits:
        return []

    def priority(path: Path) -> int:
        p = path.as_posix().lower()
        if domain == "lymph":
            if "/examples/nodej/lymph/" in p:
                return 0
            if "/examples/nodej/core/" in p:
                return 1
            return 2
        if "/examples/nodej/core/" in p:
            return 0
        if "/examples/nodej/lymph/" in p:
            return 1
        return 2

    ordered = sorted(hits, key=lambda x: (priority(x[0]), x[0].as_posix(), x[1]))
    selected: List[Tuple[Path, int, int]] = []
    used_files: set[str] = set()
    for path, line in ordered:
        file_key = path.as_posix()
        if file_key in used_files:
            continue
        used_files.add(file_key)
        start, end = compute_snippet_window(path, line)
        selected.append((path, start, end))
        if len(selected) >= max_examples:
            break
    return selected


def parse_type_fields(lines: List[str], start_index: int, kind: str) -> List[str]:
    fields: List[str] = []
    depth = 1
    j = start_index + 1

    while j < len(lines) and depth > 0:
        raw = lines[j]
        s = strip_comments(raw).strip()

        if kind == "enum" and depth == 1:
            cleaned = s.rstrip(",")
            if cleaned and cleaned not in {"{", "}"} and not cleaned.startswith(("#", "template<", "template <")):
                if "(" not in cleaned and "=" in cleaned or re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", cleaned):
                    fields.append(cleaned)

        if kind in {"struct", "class"} and depth == 1:
            if s.endswith(";"):
                if not s.startswith(("using ", "typedef ", "friend ", "static_assert", "#")):
                    # Accept only declaration-looking member fields.
                    if re.match(
                        r"^[A-Za-z_:][A-Za-z0-9_:<>,\s*&\[\]]*\s+[A-Za-z_][A-Za-z0-9_]*\s*(?:=\s*[^;]+|\{[^;]*\})?;$",
                        s,
                    ):
                        fields.append(normalize_ws(s.rstrip(";")))

        depth += raw.count("{") - raw.count("}")
        j += 1

    return fields


def parse_header(domain: str, header: Path) -> Tuple[List[ApiEntry], List[TypeEntry]]:
    lines = header.read_text(encoding="utf-8", errors="ignore").splitlines()
    api: List[ApiEntry] = []
    types: List[TypeEntry] = []

    brace_depth = 0
    class_stack: List[dict] = []
    i = 0
    in_define = False

    def in_public_class() -> Optional[str]:
        if not class_stack:
            return None
        top = class_stack[-1]
        if top.get("access") == "public":
            return top.get("name")
        return None

    while i < len(lines):
        raw = lines[i]
        s = strip_comments(raw).strip()

        if in_define:
            if not raw.rstrip().endswith("\\"):
                in_define = False
            i += 1
            continue

        if s.startswith("#define"):
            if raw.rstrip().endswith("\\"):
                in_define = True
            i += 1
            continue

        if s.startswith("#"):
            i += 1
            continue

        # Access labels inside classes/structs.
        if class_stack and s in {"public:", "private:", "protected:"}:
            class_stack[-1]["access"] = s[:-1]
            i += 1
            continue

        # using alias
        m_alias = re.match(r"^using\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+);$", s)
        if m_alias and not class_stack:
            alias_name = m_alias.group(1)
            alias_target = normalize_ws(m_alias.group(2))
            types.append(
                TypeEntry(
                    domain=domain,
                    kind="alias",
                    name=alias_name,
                    header=header,
                    line=i + 1,
                    alias_target=alias_target,
                )
            )
            i += 1
            continue

        # class/struct/enum type blocks
        m_type = re.match(r"^(class|struct|enum(?:\s+class)?)\s+([A-Za-z_][A-Za-z0-9_]*)\b", s)
        if m_type and "{" in s:
            raw_kind = m_type.group(1)
            kind = "enum" if raw_kind.startswith("enum") else raw_kind
            name = m_type.group(2)
            start_line = i + 1

            fields = parse_type_fields(lines, i, kind)
            if kind in {"struct", "enum"}:
                types.append(
                    TypeEntry(
                        domain=domain,
                        kind="struct" if kind == "struct" else "enum",
                        name=name,
                        header=header,
                        line=start_line,
                        fields=fields,
                    )
                )

            if kind in {"class", "struct"}:
                class_stack.append(
                    {
                        "name": name,
                        "kind": kind,
                        "depth": brace_depth + s.count("{") - s.count("}"),
                        "access": "private" if kind == "class" else "public",
                    }
                )

        # function/method candidate
        is_candidate = (
            "(" in s
            and not s.startswith(("if ", "for ", "while ", "switch ", "return ", "else", "template<", "template <"))
            and not s.startswith(("using ", "typedef ", "friend ", "static_assert", "namespace "))
            and not is_macro_invocation(s)
        )

        if is_candidate:
            start = i
            buf: List[str] = []
            paren_balance = 0
            terminated = False
            j = i

            while j < len(lines):
                ln = strip_comments(lines[j]).strip()
                if ln:
                    buf.append(ln)
                    paren_balance += ln.count("(") - ln.count(")")

                if buf and paren_balance <= 0 and (";" in ln or "{" in ln):
                    terminated = True
                    break
                j += 1

            if terminated:
                sig_raw = normalize_ws(" ".join(buf))
                if "{" in sig_raw:
                    sig_raw = sig_raw.split("{", 1)[0].strip()
                sig_raw = sig_raw.rstrip(";").strip()

                if (
                    sig_raw
                    and "(" in sig_raw
                    and ")" in sig_raw
                    and "?" not in sig_raw
                    and not sig_raw.startswith(("if ", "for ", "while ", "switch ", "return "))
                    and " PLUSS_" not in f" {sig_raw}"
                    and not sig_raw.startswith("PLUSS_")
                    and "#" not in sig_raw
                ):
                    first_paren = sig_raw.find("(")
                    first_eq = sig_raw.find("=")
                    if first_eq != -1 and first_eq < first_paren:
                        name_match = None
                    else:
                        name_match = re.search(r"([~A-Za-z_][A-Za-z0-9_:]*)\s*\(", sig_raw)
                    if name_match:
                        full_name = name_match.group(1)
                        base_name = full_name.split("::")[-1]

                        if base_name not in {"if", "for", "while", "switch", "return"} and "operator" not in base_name:
                            owner = None
                            kind = "function"
                            if "::" in full_name:
                                owner = "::".join(full_name.split("::")[:-1])
                                kind = "method"
                            else:
                                owner_name = in_public_class()
                                if owner_name is not None:
                                    owner = owner_name
                                    kind = "method"

                            allowed = True
                            if kind == "method" and "::" not in full_name:
                                allowed = in_public_class() is not None

                            if allowed:
                                display = sig_raw
                                if kind == "method" and owner is not None:
                                    ctor = base_name in {owner, f"~{owner}"}
                                    if not ctor:
                                        mm = re.match(rf"^(.*?)\b{re.escape(base_name)}\s*\((.*)\)\s*(.*)$", sig_raw)
                                        if mm:
                                            ret = mm.group(1).strip()
                                            args = mm.group(2).strip()
                                            tail = mm.group(3).strip()
                                            if owner and ret.endswith(f"{owner}::"):
                                                ret = ret[: -len(f"{owner}::")].strip()
                                            args_self = "self" if not args else f"self, {args}"
                                            display = normalize_ws(f"{ret} {owner}::{base_name}({args_self}) {tail}")

                                is_static_method = bool(
                                    kind == "method"
                                    and re.search(r"\bstatic\b", sig_raw) is not None
                                )

                                api.append(
                                    ApiEntry(
                                        domain=domain,
                                        kind=kind,
                                        owner=owner,
                                        name=base_name,
                                        signature=display,
                                        header=header,
                                        line=start + 1,
                                        is_static_method=is_static_method,
                                    )
                                )

                # Skip inline function body to avoid collecting statements inside it.
                term_line = strip_comments(lines[j]).strip()
                if "{" in term_line:
                    body_depth = 0
                    k = start
                    while k <= j:
                        body_depth += lines[k].count("{") - lines[k].count("}")
                        k += 1
                    while j + 1 < len(lines) and body_depth > 0:
                        j += 1
                        body_depth += lines[j].count("{") - lines[j].count("}")

                i = j

        brace_depth += raw.count("{") - raw.count("}")
        while class_stack and brace_depth < class_stack[-1]["depth"]:
            class_stack.pop()

        i += 1

    return api, types


def api_key(entry: ApiEntry) -> str:
    return f"{entry.owner}::{entry.name}" if entry.owner else entry.name


def format_owner_for_example(owner: Optional[str]) -> str:
    if not owner:
        return "obj"
    n = re.sub(r"[^A-Za-z0-9]", "_", owner).strip("_")
    if not n:
        return "obj"
    return n[0].lower() + n[1:]


def type_note(t: TypeEntry) -> str:
    specific: Dict[str, str] = {
        "LabeledBatch": (
            "Container for supervised batches: features (`x`) plus aligned labels (`y`) and optional `sample_weights`. "
            "Use this as the primary input to training, validation, and tuning workflows."
        ),
        "FeatureBatch": (
            "Container for model input features and their observation layout. "
            "Use `layout` to declare whether rows or columns represent observations."
        ),
        "ModelSpec": (
            "Model construction contract. Set task, family, and family-specific options before calling `make_classifier`/`make_regressor`."
        ),
        "TransformSpec": (
            "Transformer construction contract. Choose a transform kind and configure its options before adding it to a flow/workflow."
        ),
    }
    if t.name in specific:
        return specific[t.name]
    if t.kind == "enum":
        return "Enumerated configuration choices used by NodeJ/Lymph APIs."
    if t.kind == "alias":
        return "Type alias exported for user-facing API ergonomics."
    if t.name.endswith("Options"):
        return "Configuration object; set fields before passing into API calls."
    if t.name.endswith("Report"):
        return "Structured report object carrying metrics/diagnostics from training/evaluation routines."
    return "Public user-facing type used directly in NodeJ/Lymph workflows."


def is_user_type_page(t: TypeEntry) -> bool:
    if t.kind not in {"struct", "class"}:
        return False
    n = t.name
    if not n or n.startswith("_") or n.endswith("_"):
        return False
    # Keep option/config enums inside method pages rather than standalone pages.
    if n.endswith("Options") or n.endswith("Policy"):
        return False
    if "Factory" in n or "Wrapper" in n:
        return False
    return True


def member_name_from_field(t: TypeEntry, field_text: str, fallback_idx: int) -> str:
    if t.kind == "enum":
        base = field_text.split("=", 1)[0].strip().rstrip(",")
        return base or f"entry_{fallback_idx}"
    m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:=.+)?$", field_text.strip())
    if m:
        return m.group(1)
    return f"field_{fallback_idx}"


def fallback_example(entry: ApiEntry) -> List[str]:
    lines: List[str] = []
    lines.append("```cpp")
    lines.append("// Minimal call skeleton. Replace placeholders with concrete data/model objects.")
    if entry.kind == "method" and entry.owner:
        if entry.is_static_method:
            lines.append(f"auto out = {entry.owner}::{entry.name}(/* args */);")
        else:
            var = format_owner_for_example(entry.owner)
            lines.append(f"{entry.owner} {var}{{}};")
            lines.append(f"auto out = {var}.{entry.name}(/* args */);")
    else:
        lines.append(f"auto out = {entry.name}(/* args */);")
    lines.append("```")
    return lines


def fallback_type_example(t: TypeEntry) -> List[str]:
    lines: List[str] = []
    lines.append("```cpp")
    if t.kind == "alias":
        lines.append(f"{t.name} value{{}};")
    elif t.kind == "enum":
        if t.fields:
            first = t.fields[0].split("=", 1)[0].strip()
            lines.append(f"auto mode = {t.name}::{first};")
        else:
            lines.append(f"// choose a {t.name} enumerator")
    else:
        lines.append(f"{t.name} cfg{{}};")
        if t.fields:
            first = t.fields[0]
            name_match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:=.+)?$", first)
            if name_match:
                lines.append(f"// cfg.{name_match.group(1)} = ...;")
    lines.append("```")
    return lines


def collect_demo_outputs() -> Dict[str, Path]:
    RUN_LOG_DIR.mkdir(parents=True, exist_ok=True)
    for stale in RUN_LOG_DIR.glob("*.out"):
        stale.unlink(missing_ok=True)
    by_source: Dict[str, Path] = {}
    exe_roots = [p for p in BUILD_EXE_ROOTS if p.exists()]
    if not exe_roots:
        return by_source

    exes: List[Path] = []
    for root in exe_roots:
        exes.extend(p for p in root.rglob("plussai_*") if p.is_file() and os.access(p, os.X_OK))
    exes = sorted(set(exes))
    for exe in exes:
        if any(k in exe.name for k in ("end_to_end", "workflow_showcase", "bike_sharing")):
            continue
        out_file = RUN_LOG_DIR / f"{exe.name}.out"
        text = ""
        try:
            cp = subprocess.run(
                [str(exe)],
                cwd=str(ROOT),
                text=True,
                capture_output=True,
                timeout=90,
                check=False,
            )
            text = (cp.stdout or "") + (cp.stderr or "")
            if cp.returncode != 0:
                text += f"\n[exit_code] {cp.returncode}\n"
        except subprocess.TimeoutExpired:
            text = "[timeout] demo execution exceeded 90s\n"
        except Exception as ex:  # pragma: no cover
            text = f"[error] unable to run demo: {ex}\n"

        text = text.replace(str(ROOT), "<repo_root>")
        sanitized = [sanitize_output_line(ln) for ln in text.splitlines()]
        text = "\n".join(sanitized)
        out_file.write_text(text.strip() + "\n", encoding="utf-8")

    for src in EXAMPLE_FILES:
        stem = src.stem
        binary_name = f"plussai_{stem}"
        out_file = RUN_LOG_DIR / f"{binary_name}.out"
        if out_file.exists():
            excerpt = read_output_excerpt(out_file)
            if output_is_usable(excerpt):
                by_source[src.as_posix()] = out_file

    return by_source


def read_output_excerpt(path: Optional[Path], max_lines: int = 12) -> Optional[List[str]]:
    if path is None or not path.exists():
        return None
    lines = [ln.rstrip() for ln in path.read_text(encoding="utf-8", errors="ignore").splitlines()]
    while lines and not lines[0].strip():
        lines.pop(0)
    if not lines:
        return None
    cut = lines[:max_lines]
    if len(lines) > max_lines:
        cut.append("...")
    return cut


def read_code_excerpt(path: Path, start: int, end: int) -> List[str]:
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    lo = max(1, start)
    hi = min(len(lines), end)
    excerpt = [ln.replace(str(ROOT), "<repo_root>") for ln in lines[lo - 1 : hi]]
    # Trim leading/trailing blank lines for cleaner docs blocks.
    while excerpt and not excerpt[0].strip():
        excerpt.pop(0)
    while excerpt and not excerpt[-1].strip():
        excerpt.pop()
    while excerpt and re.match(r"^\s*(template\s*<|using namespace )", excerpt[0]):
        excerpt.pop(0)
    return excerpt


def output_is_usable(lines: Optional[List[str]]) -> bool:
    if not lines:
        return False
    first = lines[0].strip().lower()
    if first.startswith("[timeout]") or first.startswith("[error]"):
        return False
    return True


def excerpt_is_useful(excerpt: List[str], token: str) -> bool:
    if not excerpt:
        return False
    noisy_prefixes = ("template <", "class ", "struct ", "namespace ")
    noisy_lines = 0
    for ln in excerpt:
        s = ln.strip()
        if any(s.startswith(p) for p in noisy_prefixes):
            noisy_lines += 1
    if noisy_lines > 0:
        return False
    token_hit = any(re.search(rf"\b{re.escape(token)}\b", ln) for ln in excerpt)
    return token_hit


def param_description(param: ParamInfo, entry: ApiEntry) -> str:
    n = param.name.lower()
    t = param.type_text.lower()
    if param.name == "self":
        return "Receiver object on which this method is invoked."
    if any(k in n for k in ("x", "feature", "batch", "frame", "matrix", "data")):
        return "Input feature data used by this call."
    if any(k in n for k in ("y", "target", "label", "truth")):
        return "Target values aligned to input observations."
    if "option" in n or n in {"opts", "cfg", "config", "train", "infer", "assess", "split"}:
        return "Configuration object controlling algorithm behavior and safety defaults."
    if any(k in n for k in ("metric", "objective", "score")):
        return "Metric selection or score-control argument for evaluation/tuning."
    if any(k in n for k in ("seed", "state", "random")):
        return "Randomization control for reproducible behavior."
    if any(k in n for k in ("path", "file", "uri")):
        return "Path to persisted model/data artifact."
    if any(k in n for k in ("fold", "trial", "k", "neighbors", "clusters", "components")):
        return "Algorithm-size/count argument used to shape fit or split behavior."
    if "span<" in t or "vector<" in t or "initializer_list" in t:
        return "List-like argument; pass values in the order expected by the signature."
    if "thread" in t or "series" in t:
        return "Pluss numeric vector/tensor input."
    if "note" in t:
        return "Categorical/text-like input."
    return "Method argument; see signature type for accepted form."


def describe_option_value(value: str) -> str:
    v = value.lower()
    if "holdout" in v:
        return "Runs a single train/validation split."
    if "kfold" in v or "cross" in v:
        return "Runs multiple folds and aggregates fold metrics."
    if "strat" in v:
        return "Preserves class balance across generated splits."
    if "regress" in v:
        return "Regression behavior."
    if "classif" in v:
        return "Classification behavior."
    if "rows" in v:
        return "Treats rows as observations."
    if "columns" in v or "cols" in v:
        return "Treats columns as observations."
    if "none" == v:
        return "Disables this behavior."
    if "auto" in v:
        return "Lets the library choose a default strategy."
    return "Accepted enum choice for this method."


def describe_option_field(field: str) -> str:
    s = field.lower()
    if re.search(r"\btask\b", s):
        return "Selects task type (for example classification vs regression)."
    if re.search(r"\bfamily\b", s):
        return "Selects the model family implementation."
    if re.search(r"\boptions\b", s):
        return "Family-specific options payload used by the selected model/transform family."
    if "feature_names" in s:
        return "Explicit list of feature columns to read from the input frame."
    if "label_feature" in s:
        return "Column used as the supervised target (`y`)."
    if "sample_weight" in s:
        return "Optional column used as per-row training weight."
    if "layout" in s:
        return "Declares whether observations are represented by rows or columns."
    if "thread_policy" in s:
        return "Controls how numeric/temporal frame columns are adapted into `Series`."
    if "note_policy" in s:
        return "Controls how note/categorical columns are handled during adaptation."
    if "seed" in s or "random" in s:
        return "Random seed for reproducible behavior."
    if "fold" in s or "k_" in s or "n_" in s:
        return "Controls fold/trial/count sizing."
    if "metric" in s or "objective" in s:
        return "Selects optimization or evaluation objective."
    if "regularization" in s:
        return "Controls regularization mode and strength."
    if "fit_intercept" in s:
        return "Whether to learn an intercept/bias term."
    if "learning_rate" in s or "eta" in s:
        return "Learning-rate step size."
    if "max_iter" in s or "iterations" in s:
        return "Maximum optimization iterations."
    if "tol" in s or "tolerance" in s:
        return "Convergence tolerance."
    if "verbose" in s:
        return "Enables additional runtime logging."
    if "path" in s or "file" in s:
        return "Filesystem location for loading/saving artifacts."
    return "Option field used by this method."


def return_description(return_type: str) -> str:
    rt = return_type.strip()
    m = re.match(r"StatusOr<\s*(.+)\s*>", rt)
    if m:
        inner = normalize_ws(m.group(1))
        return (
            f"Returns `StatusOr<{inner}>`. Check `.ok()` first; on success read `.value()` "
            "for the computed result."
        )
    if rt == "Status":
        return "Returns a status object indicating success/failure and an error message on failure."
    if rt in {"void"}:
        return "No direct return value; operation applies side effects to object state."
    if rt in {"bool", "int", "std::size_t", "std::int64_t", "double", "float"}:
        return f"Returns `{rt}`."
    return f"Returns `{rt}`."


def note_for_entry(entry: ApiEntry, sig: SignatureInfo) -> str:
    n = entry.name.lower()
    if n == "labeled_batch_from_dataframe":
        return (
            "Builds a supervised `LabeledBatch` from a DataFrame. "
            "Use `options` to pick feature columns, label column, weight column, and categorical handling policy."
        )
    if n == "slice_labeled_batch":
        return "Returns a row-subset of a labeled batch using the provided index selection."
    if n == "slice_feature_batch":
        return "Returns a row-subset of a feature-only batch using the provided index selection."
    if "from_dataframe" in n:
        return "Adapts DataFrame columns into NodeJ batch objects for training/inference."
    if n == "make_flow":
        return "Constructs a `Flow` from model spec/options; use it to chain transforms and model training."
    if n == "make_workflow":
        return "Constructs a high-level workflow object with model and transform stages."
    if n in {"make_classifier", "make_regressor", "make_transformer"}:
        return "Factory entry point that validates spec/options and returns a ready-to-train component."
    if "assess" in n:
        return "Computes evaluation metrics from predictions and truth using the selected report options."
    if "tune" in n or "sweep" in n or "probe" in n:
        return "Runs parameter search and returns per-trial metrics plus best-trial selection."
    if n.startswith("set_"):
        return "Sets or updates internal configuration/state used by later train/infer/evaluate calls."
    if n.startswith("get_"):
        return "Returns a derived/view value from current object state without mutating state."
    if n.startswith("has_"):
        return "Boolean introspection helper that reports whether a capability/state is available."
    if n in {"defined", "ok", "trained"}:
        return "Boolean state check for object readiness."
    if "validate" in n:
        return "Validates shape/layout/typing invariants for this data container before downstream use."
    if n in {"sample_count", "feature_count", "stage_count", "seen_samples"}:
        return "Reports a cardinality/count statistic from the current object state."
    if "learn" in n or "train" in n:
        return (
            "Fits model/transform state from the provided training data. "
            "Call this before inference-oriented methods."
        )
    if "infer" in n or "predict" in n:
        return "Computes predictions from prepared inputs using the currently learned state."
    if "score" in n and "infer" in n:
        return "Returns decision/probability-like scores rather than thresholded labels."
    if "assess" in n or "metric" in n:
        return "Evaluates model output against targets and returns metric aggregates."
    if "split" in n:
        return "Builds train/test index partitions according to split-plan options."
    if "cross" in n or "cv" in n:
        return "Runs cross-validation and aggregates fold-level diagnostics."
    if "tune" in n or "sweep" in n or "probe" in n:
        return "Executes hyperparameter search and reports trial-by-trial objective values."
    if "save" in n:
        return "Persists the current fitted artifact so it can be loaded for inference later."
    if "load" in n:
        return "Loads a previously persisted artifact and restores its inference-ready state."
    if any(k in n for k in ("imputer", "scaler", "encoder", "normalize", "map", "binarizer")):
        return "Applies preprocessing/transformation behavior configured by the current options."
    if "factory" in (entry.owner or "").lower() or "factory" in n:
        return "Factory helper that constructs a configured model/workflow object."
    if entry.kind == "method":
        return "Instance method on `self`."
    return "Namespace-level API function."


def render_inputs_block(signatures: List[SignatureInfo], entry: ApiEntry) -> List[str]:
    lines: List[str] = []
    lines.append("## Inputs")
    lines.append("")
    param_bucket: Dict[str, Dict[str, set]] = {}
    for sig in signatures:
        for p in sig.params:
            if p.name == "self":
                continue
            b = param_bucket.setdefault(p.name, {"types": set(), "defaults": set()})
            b["types"].add(normalize_ws(clean_type_for_docs(p.type_text)))
            if p.default:
                b["defaults"].add(p.default)
    if not param_bucket:
        lines.append("None.")
        lines.append("")
        return lines
    for pname in sorted(param_bucket.keys(), key=str.lower):
        info = param_bucket[pname]
        types_join = " | ".join(sorted(info["types"]))
        defaults_join = ", ".join(sorted(info["defaults"])) if info["defaults"] else None
        if defaults_join:
            lines.append(f"- `{pname}` (`{types_join}`), default: `{defaults_join}`")
        else:
            lines.append(f"- `{pname}` (`{types_join}`)")
        lines.append(f"  {param_description(ParamInfo(name=pname, type_text=types_join), entry)}")
    lines.append("")
    return lines


def render_output_block(signatures: List[SignatureInfo]) -> List[str]:
    lines: List[str] = []
    lines.append("## Output")
    lines.append("")
    returns = sorted({clean_type_for_docs(s.return_type) for s in signatures if s.return_type})
    if returns:
        for r in returns:
            lines.append(f"- {return_description(r)}")
    else:
        lines.append("- Returns method result for the selected overload.")
    lines.append("")
    return lines


def synthesized_output_lines(entry: ApiEntry, sig: SignatureInfo) -> List[str]:
    ret = clean_type_for_docs(sig.return_type)
    n = entry.name.lower()
    if ret.startswith("StatusOr<"):
        return [
            f"{entry.name}: ok",
            "status.ok() == true",
            "result.value() is available",
        ]
    if ret == "Status":
        return [
            f"{entry.name}: ok",
            "status.ok() == true",
        ]
    if ret == "bool":
        return [f"{entry.name}: true"]
    if ret in {"double", "float", "int", "std::size_t", "std::int64_t"}:
        return [f"{entry.name}: <numeric_result>"]
    if ret == "void":
        return [f"{entry.name} completed"]
    if "report" in ret.lower() or "metrics" in n or "assess" in n:
        return [
            "metrics: <metric_1> <metric_2> ...",
            "evaluation completed",
        ]
    if "split" in n:
        return [
            "split completed",
            "train/test partitions ready",
        ]
    return [f"{entry.name} returned {ret or 'result'}"]


def generated_usage_example(entry: ApiEntry, sig: SignatureInfo) -> List[str]:
    lines: List[str] = []
    lines.append("### Example 1 (Minimal Usage)")
    lines.append("")
    lines.append("```cpp")
    lines.append("pluss::OutputEngine out;")
    call_args: List[str] = []
    for p in sig.params:
        if p.name == "self":
            continue
        call_args.append(f"/* {p.name} */")
    arg_list = ", ".join(call_args)
    call_expr = f"{sig.qualified_name}({arg_list})"
    ret = clean_type_for_docs(sig.return_type)
    if ret.startswith("StatusOr<"):
        lines.append(f"auto res = {call_expr};")
        lines.append("if (!res.ok()) {")
        lines.append(f'  out.error("{entry.name} failed", res.status().message);')
        lines.append("} else {")
        lines.append(f'  out.info("{entry.name} ok");')
        lines.append("}")
    elif ret == "Status":
        lines.append(f"auto st = {call_expr};")
        lines.append("if (!st.ok()) {")
        lines.append(f'  out.error("{entry.name} failed", st.message);')
        lines.append("} else {")
        lines.append(f'  out.info("{entry.name} ok");')
        lines.append("}")
    elif ret == "void":
        lines.append(f"{call_expr};")
        lines.append(f'out.info("{entry.name} completed");')
    else:
        lines.append(f"auto value = {call_expr};")
        lines.append(f'out.info("{entry.name}", value);')
    lines.append("```")
    lines.append("")
    lines.append("**Expected output**")
    lines.append("")
    lines.append("```text")
    lines.extend(synthesized_output_lines(entry, sig))
    lines.append("```")
    lines.append("")
    return lines


def render_parameters_block(sig: SignatureInfo, entry: ApiEntry) -> List[str]:
    lines: List[str] = []
    lines.append("## Parameters")
    lines.append("")
    if not sig.params:
        lines.append("None.")
        lines.append("")
        return lines
    for p in sig.params:
        type_text = p.type_text if p.name != "self" else "receiver"
        optional = f", default: `{p.default}`" if p.default else ""
        lines.append(f"- `{p.name}` : `{type_text}`{optional}")
        lines.append(f"  {param_description(p, entry)}")
    lines.append("")
    return lines


def related_entries(entries: List[ApiEntry], entry: ApiEntry, limit: int = 5) -> List[ApiEntry]:
    this_key = api_key(entry)
    cands = [e for e in entries if api_key(e) != this_key and (e.owner == entry.owner or e.name == entry.name)]
    return sorted(cands, key=lambda e: api_key(e).lower())[:limit]


def short_type_name(type_text: str) -> str:
    t = normalize_ws(type_text)
    t = re.sub(r"\bconst\b", "", t)
    t = t.replace("&", " ").replace("*", " ")
    t = normalize_ws(t)
    m = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*(?:<.*>)?$", t)
    return m.group(1) if m else t


def render_options_block(
    owner: Optional[str],
    signatures: List[SignatureInfo],
    type_lookup: Dict[str, TypeEntry],
) -> List[str]:
    lines: List[str] = []
    used: Dict[str, TypeEntry] = {}

    def try_add(type_name: str) -> None:
        if type_name in type_lookup:
            t = type_lookup[type_name]
            if t.kind in {"struct", "enum"} and t.fields:
                used[type_name] = t

    if owner:
        try_add(owner.split("::")[-1])

    for sig in signatures:
        for p in sig.params:
            if p.name == "self":
                continue
            try_add(short_type_name(p.type_text))

    if not used:
        lines.append("## Options")
        lines.append("")
        lines.append("No explicit option struct/enum parameters were detected for this method.")
        lines.append("")
        return lines

    lines.append("## Options")
    lines.append("")
    lines.append("Option semantics used by this method:")
    lines.append("")
    for name in sorted(used.keys(), key=str.lower):
        t = used[name]
        if t.kind == "enum":
            lines.append(f"- `{name}` (`enum`):")
            for f in t.fields[:24]:
                val = f.split("=", 1)[0].strip().rstrip(",")
                if not val:
                    continue
                lines.append(f"  - `{val}`: {describe_option_value(val)}")
        else:
            lines.append(f"- `{name}` (`struct`):")
            for f in t.fields[:32]:
                field = normalize_ws(f).rstrip(";")
                lines.append(f"  - `{field}`")
                lines.append(f"    {describe_option_field(field)}")
        lines.append("")
    return lines


def is_public_user_entry(entry: ApiEntry) -> bool:
    name = entry.name
    owner = entry.owner or ""
    sig = entry.signature
    nlow = name.lower()
    olow = owner.lower()

    if not name or name.startswith("_") or nlow.endswith("_"):
        return False
    if "detail" in olow or "::detail" in olow:
        return False
    if "undefined" == nlow:
        return False
    if owner in {"Status"} and nlow in {
        "internal",
        "precondition",
        "invalid_argument",
        "shape_mismatch",
        "numerical_failure",
        "not_implemented",
    }:
        return False
    if nlow in {"resolve", "must"}:
        return False
    if "WrapperName" in owner or "WrapperName" in sig:
        return False
    if "= delete" in sig or "= default" in sig:
        return False
    return True


def write_lines(path: Path, lines: Iterable[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def render_domain(
    domain: str,
    title: str,
    intro: str,
    api_entries: List[ApiEntry],
    type_entries: List[TypeEntry],
    output_landing: Path,
    token_map: Dict[str, List[Tuple[Path, int]]],
    source_output_map: Dict[str, Path],
) -> Dict[str, int]:
    domain_root = DOCS_DIR / "generated" / domain
    if domain_root.exists():
        shutil.rmtree(domain_root)
    (domain_root / "api").mkdir(parents=True, exist_ok=True)

    # Filter internal helpers
    internal_name_prefixes = (
        "prep_",
        "cross_validate_default_",
        "metric_report_from_series_",
        "scalar_metric_",
        "minimize_metric_",
    )

    api_entries = [
        e
        for e in api_entries
        if not e.name.startswith(internal_name_prefixes)
        and is_public_user_entry(e)
    ]

    type_entries = [
        t for t in type_entries
        if not t.name.endswith("_")
        and t.name not in {"WrapperName"}
        and not t.name.startswith("_")
    ]

    # Dedup
    seen_api = set()
    dedup_api: List[ApiEntry] = []
    for e in api_entries:
        k = (e.owner or "", e.name, e.signature, rel_to_root(e.header), e.line)
        if k in seen_api:
            continue
        seen_api.add(k)
        dedup_api.append(e)

    seen_types = set()
    dedup_types: List[TypeEntry] = []
    for t in type_entries:
        k = (t.kind, t.name, rel_to_root(t.header), t.line, t.alias_target or "")
        if k in seen_types:
            continue
        seen_types.add(k)
        dedup_types.append(t)

    api_entries = sorted(dedup_api, key=lambda e: (api_key(e).lower(), e.signature.lower()))
    type_entries = sorted(dedup_types, key=lambda t: (t.name.lower(), t.kind))
    type_lookup: Dict[str, TypeEntry] = {}
    for t in type_entries:
        if t.name not in type_lookup:
            type_lookup[t.name] = t

    # API index
    groups: Dict[str, List[ApiEntry]] = {}
    for e in api_entries:
        groups.setdefault(api_key(e), []).append(e)

    api_pages: List[str] = []
    for key in sorted(groups.keys(), key=str.lower):
        group = sorted(groups[key], key=lambda e: e.signature.lower())
        slug = slugify(f"method-{key}")
        rel = f"generated/{domain}/api/{slug}"
        api_pages.append(rel)

        signatures = [parse_signature_info(e.signature.rstrip(";") + ";") for e in group]
        lines: List[str] = []
        lines.append(f"# `{key}`")
        lines.append("")
        lines.append("## API Signature")
        lines.append("")
        for sig in signatures:
            lines.append(f"- `{public_signature_line(sig)}`")
        lines.append("")

        lines.append("## Notes")
        lines.append("")
        lines.append(note_for_entry(group[0], signatures[0]))
        lines.append("")

        lines.extend(render_inputs_block(signatures, group[0]))
        lines.extend(render_output_block(signatures))

        lines.extend(render_options_block(group[0].owner, signatures, type_lookup))

        lines.append("## Examples")
        lines.append("")
        lines.extend(generated_usage_example(group[0], signatures[0]))

        raw_examples = choose_examples(token_map, domain, group[0].name, max_examples=8)
        examples: List[Tuple[Path, int, int, Optional[List[str]]]] = []
        for p, st, en in raw_examples:
            excerpt = read_output_excerpt(source_output_map.get(p.as_posix()))
            examples.append((p, st, en, excerpt))
        examples.sort(key=lambda x: (0 if output_is_usable(x[3]) else 1, x[0].as_posix(), x[1]))
        with_output = [e for e in examples if output_is_usable(e[3])]
        if with_output:
            examples = with_output[:2]
        else:
            examples = examples[:1]

        if examples:
            for ex_idx, (p, st, en, out_excerpt) in enumerate(examples, start=2):
                code_excerpt = read_code_excerpt(p, st, en)
                if not excerpt_is_useful(code_excerpt, group[0].name):
                    continue
                lines.append(f"### Example {ex_idx} (Demo Pattern)")
                lines.append("")
                lines.append("```cpp")
                lines.extend(code_excerpt)
                lines.append("```")
                lines.append("")
                lines.append("**Expected output**")
                lines.append("")
                lines.append("```text")
                if output_is_usable(out_excerpt):
                    lines.extend([sanitize_output_line(ln) for ln in (out_excerpt or [])])
                else:
                    lines.extend(synthesized_output_lines(group[0], signatures[0]))
                lines.append("```")
                lines.append("")
        write_lines(DOCS_DIR / f"{rel}.md", lines)

    # Data-type pages (user-facing structs/classes only; option enums stay inside method pages).
    type_pages: List[str] = []
    for t in sorted(type_entries, key=lambda x: x.name.lower()):
        if not is_user_type_page(t):
            continue
        rel = f"generated/{domain}/api/{slugify(f'type-{t.name}')}"
        type_pages.append(rel)

        tl: List[str] = []
        tl.append(f"# `{t.name}`")
        tl.append("")
        tl.append("## API Signature")
        tl.append("")
        tl.append(f"- `{t.name}`")
        tl.append("")
        tl.append("## Inputs")
        tl.append("")
        tl.append("- This is a public data type. Populate its fields before passing into APIs.")
        tl.append("")
        tl.append("## Output")
        tl.append("")
        tl.append(f"- A `{t.name}` object used by NodeJ/Lymph workflows.")
        tl.append("")
        tl.append("## Notes")
        tl.append("")
        tl.append(type_note(t))
        tl.append("")
        if t.fields:
            tl.append("## Fields")
            tl.append("")
            tl.append("Public fields on this type:")
            tl.append("")
            for f in t.fields[:40]:
                field = normalize_ws(f).rstrip(";")
                tl.append(f"- `{field}`")
            tl.append("")

        tl.append("## Examples")
        tl.append("")
        raw_examples = choose_examples(token_map, domain, t.name, max_examples=6, for_type=True)
        examples: List[Tuple[Path, int, int, Optional[List[str]]]] = []
        for p, st, en in raw_examples:
            excerpt = read_output_excerpt(source_output_map.get(p.as_posix()))
            examples.append((p, st, en, excerpt))
        examples.sort(key=lambda x: (0 if output_is_usable(x[3]) else 1, x[0].as_posix(), x[1]))
        with_output = [e for e in examples if output_is_usable(e[3])]
        if with_output:
            examples = with_output[:3]
        else:
            examples = examples[:2]

        if examples:
            for ex_idx, (p, st, en, out_excerpt) in enumerate(examples, start=1):
                code_excerpt = read_code_excerpt(p, st, en)
                if not excerpt_is_useful(code_excerpt, t.name):
                    continue
                tl.append(f"### Example {ex_idx}")
                tl.append("")
                tl.append("```cpp")
                tl.extend(code_excerpt)
                tl.append("```")
                tl.append("")
                tl.append("**Expected output**")
                tl.append("")
                tl.append("```text")
                if output_is_usable(out_excerpt):
                    tl.extend([sanitize_output_line(ln) for ln in (out_excerpt or [])])
                else:
                    tl.append(f"{t.name} configured")
                tl.append("```")
                tl.append("")
        else:
            tl.extend(fallback_type_example(t))
            tl.append("")
        write_lines(DOCS_DIR / f"{rel}.md", tl)

    api_index_lines: List[str] = []
    api_index_lines.append(f"# {title}: Method Index")
    api_index_lines.append("")
    api_index_lines.append(f"Total method pages: **{len(api_pages)}**")
    api_index_lines.append(f"Total data-type pages: **{len(type_pages)}**")
    api_index_lines.append("")
    if type_pages:
        api_index_lines.append("## Data Types")
        api_index_lines.append("")
        api_index_lines.append("```{toctree}")
        api_index_lines.append(":maxdepth: 1")
        api_index_lines.append("")
        for p in type_pages:
            api_index_lines.append(p.replace(f"generated/{domain}/", ""))
        api_index_lines.append("```")
        api_index_lines.append("")

    api_index_lines.append("## Methods")
    api_index_lines.append("")
    api_index_lines.append("```{toctree}")
    api_index_lines.append(":maxdepth: 1")
    api_index_lines.append("")
    for p in api_pages:
        api_index_lines.append(p.replace(f"generated/{domain}/", ""))
    api_index_lines.append("```")
    write_lines(domain_root / "api_index.md", api_index_lines)

    # Domain landing page
    landing_lines: List[str] = []
    landing_lines.append(f"# {title}")
    landing_lines.append("")
    landing_lines.append(intro.strip())
    landing_lines.append("")
    landing_lines.append(f"- Method pages: **{len(api_pages)}**")
    landing_lines.append(f"- Data-type pages: **{len(type_pages)}**")
    landing_lines.append(f"- Overloads covered: **{len(api_entries)}**")
    landing_lines.append("")
    landing_lines.append("## Function and Method Reference")
    landing_lines.append("")
    landing_lines.append("```{toctree}")
    landing_lines.append(":maxdepth: 1")
    landing_lines.append("")
    landing_lines.append(f"generated/{domain}/api_index")
    landing_lines.append("```")
    write_lines(output_landing, landing_lines)

    return {
        "types": len(type_pages),
        "type_members": 0,
        "symbols": 0,
        "api": len(api_pages),
        "params": 0,
        "total": len(api_pages) + len(type_pages),
    }


def main() -> None:
    token_map = find_example_occurrences()
    source_output_map = collect_demo_outputs()

    nodej_api: List[ApiEntry] = []
    nodej_types: List[TypeEntry] = []
    for h in NODEJ_HEADERS:
        a, t = parse_header("nodej", h)
        nodej_api.extend(a)
        nodej_types.extend(t)

    lymph_api: List[ApiEntry] = []
    lymph_types: List[TypeEntry] = []
    for h in LYMPH_HEADERS:
        a, t = parse_header("lymph", h)
        lymph_api.extend(a)
        lymph_types.extend(t)

    nodej_counts = render_domain(
        "nodej",
        "NodeJ Full API Reference",
        """
NodeJ method-centric reference generated from public headers.
Each page follows **API Signature -> Inputs -> Output -> Options -> Examples (with expected outputs)**.
Options/enums are documented under the methods that consume them.
""",
        nodej_api,
        nodej_types,
        DOCS_DIR / "nodej_full_api.md",
        token_map,
        source_output_map,
    )

    lymph_counts = render_domain(
        "lymph",
        "Lymph Full API Reference",
        """
Lymph method-centric reference generated from public headers.
Each page follows **API Signature -> Inputs -> Output -> Options -> Examples (with expected outputs)**.
Options/enums are documented under the methods that consume them.
""",
        lymph_api,
        lymph_types,
        DOCS_DIR / "lymph_full_api.md",
        token_map,
        source_output_map,
    )

    api_ref_lines = [
        "# API Reference",
        "",
        "- [NodeJ Overview](nodej_overview.md)",
        "- [Lymph Overview](lymph_overview.md)",
        "- [NodeJ Full API Reference](nodej_full_api.md)",
        "- [Lymph Full API Reference](lymph_full_api.md)",
        "",
        "## Coverage Snapshot",
        "",
        f"- NodeJ pages: **{nodej_counts['total']}** (methods: {nodej_counts['api']}, data-types: {nodej_counts['types']})",
        f"- Lymph pages: **{lymph_counts['total']}** (methods: {lymph_counts['api']}, data-types: {lymph_counts['types']})",
        f"- Combined pages: **{nodej_counts['total'] + lymph_counts['total']}**",
    ]
    write_lines(DOCS_DIR / "api_reference.md", api_ref_lines)

    print(f"nodej_pages={nodej_counts['total']} (types={nodej_counts['types']} type_members={nodej_counts['type_members']} symbols={nodej_counts['symbols']} api={nodej_counts['api']} params={nodej_counts['params']})")
    print(f"lymph_pages={lymph_counts['total']} (types={lymph_counts['types']} type_members={lymph_counts['type_members']} symbols={lymph_counts['symbols']} api={lymph_counts['api']} params={lymph_counts['params']})")
    print(f"combined_pages={nodej_counts['total'] + lymph_counts['total']}")


if __name__ == "__main__":
    main()
