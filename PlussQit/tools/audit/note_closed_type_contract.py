#!/usr/bin/env python3
"""Generate and verify the closed-Note public return contract."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GENERATED_DECLS = ROOT / "src/CodeN/generated/NoteMethodDeclsGen.h"
NLP_DECLS = ROOT / "src/CodeN/core/NoteNlp.h"
REGEX_DECLS = ROOT / "src/CodeN/core/NoteRegex.h"
TEX_DECLS = ROOT / "src/CodeN/core/NoteTex.h"
DEFAULT_OUTPUT = ROOT / "tools/audit/note_closed_type_contract.json"
FORBIDDEN_ESCAPE_NAMES = ("contains_series", "lengths_bytes_series")
PUBLIC_IMPLEMENTATION_FILES = (
    ROOT / "src/CodeN/core/Note.h",
    ROOT / "src/CodeN/core/Note.cpp",
    ROOT / "src/CodeN/core/OpsInternal.h",
    ROOT / "src/CodeN/core/OpsInternal.cpp",
)

DECL_RE = re.compile(r"^\s*(.+?)\s+([A-Za-z_][A-Za-z0-9_]*)\((.*)\) const;\s*$")
BOOL_RESULTS = {
    "contains",
    "starts_with",
    "ends_with",
    "regex_match",
    "regex_search",
    "match",
    "search",
    "is_ascii",
    "is_utf8_valid",
    "is_blank",
    "is_alpha",
    "is_alnum",
    "is_digit",
    "is_lower",
    "is_upper",
    "is_title",
    "is_whitespace",
    "contains_any_digit",
    "contains_any_alpha",
}
INT_RESULTS = {
    "lengths_bytes",
    "lengths_chars",
    "lengths_graphemes",
    "count_substring",
    "count_regex",
    "count",
}
FLOAT_RESULTS = {
    "formality",
    "readability_grade",
    "sentiment",
    "spam_score",
    "subjectivity",
    "toxicity",
    "jaccard_acronyms",
    "jaccard_citations",
    "jaccard_math_symbols",
    "jaccard_terms",
    "jaccard_tokens",
    "overlap_acronyms",
    "overlap_citations",
    "overlap_math_symbols",
    "overlap_terms",
    "overlap_tokens",
    "nlp_formality",
    "nlp_readability_grade",
    "nlp_sentiment",
    "nlp_spam_score",
    "nlp_subjectivity",
    "nlp_toxicity",
}


def compact(value: str) -> str:
    return " ".join(value.split())


def classify_generated(name: str, current_return: str) -> dict[str, str]:
    if current_return == "::std::vector<::std::uint8_t>":
        if name == "is_bytes":
            return {
                "target_return": "removed",
                "target_dtype": "n/a",
                "contract": "hidden per-cell predicate colliding with the dtype query",
                "migration": "remove-hidden-collision",
            }
        return {
            "target_return": "Note",
            "target_dtype": "Bool",
            "contract": "shape-preserving predicate",
            "migration": "breaking-return",
        }
    if current_return == "::std::vector<::std::int64_t>":
        return {
            "target_return": "Note",
            "target_dtype": "Int64",
            "contract": "shape-preserving measurement",
            "migration": "breaking-return",
        }
    if current_return == "TokenBatch":
        return {
            "target_return": "TokenBatch",
            "target_dtype": "Text+Int64",
            "contract": "ragged result composed only of Note tokens and Note offsets",
            "migration": "complete",
        }
    if current_return == "Note":
        if name in BOOL_RESULTS:
            return {
                "target_return": "Note",
                "target_dtype": "Bool",
                "contract": "shape-preserving predicate",
                "migration": "complete",
            }
        if name in INT_RESULTS:
            return {
                "target_return": "Note",
                "target_dtype": "Int64",
                "contract": "shape-preserving measurement",
                "migration": "complete",
            }
        if name in FLOAT_RESULTS:
            return {
                "target_return": "Note",
                "target_dtype": "Float64",
                "contract": "shape-preserving numeric score",
                "migration": "complete",
            }
        return {
            "target_return": "Note",
            "target_dtype": "Text/Bytes/preserve",
            "contract": "closed Note transformation",
            "migration": "none-or-validation",
        }
    raise RuntimeError(f"unclassified generated return type: {current_return} ({name})")


def generated_records() -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    public = True
    for line_number, line in enumerate(GENERATED_DECLS.read_text(encoding="utf-8").splitlines(), 1):
        stripped = line.strip()
        if stripped == "public:":
            public = True
            continue
        if stripped == "private:":
            public = False
            continue
        match = DECL_RE.match(line)
        if not match:
            if stripped and not stripped.startswith("//"):
                raise RuntimeError(
                    f"unparsed generated declaration at {GENERATED_DECLS}:{line_number}: {stripped}"
                )
            continue
        if not public:
            continue
        current_return, name, parameters = match.groups()
        current_return = compact(current_return)
        record: dict[str, object] = {
            "source": str(GENERATED_DECLS.relative_to(ROOT)),
            "scope": "Note",
            "line": line_number,
            "name": name,
            "signature": f"{current_return} Note::{name}({compact(parameters)}) const",
            "current_return": current_return,
        }
        record.update(classify_generated(name, current_return))
        records.append(record)
    return records


def accessor_records(path: Path, class_name: str, public_scope: str) -> list[dict[str, object]]:
    records: list[dict[str, object]] = []
    inside_class = False
    public = False
    statement = ""
    statement_line = 0
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        stripped = line.strip()
        if stripped.startswith(f"class CODEN_API {class_name}"):
            inside_class = True
            continue
        if not inside_class:
            continue
        if stripped == "public:":
            public = True
            continue
        if stripped == "private:":
            public = False
            continue
        if stripped == "};":
            break
        if not public or not stripped or stripped.startswith("//"):
            continue
        if not statement:
            if "(" not in stripped:
                continue
            if re.search(rf"\b{re.escape(class_name)}\s*\(", stripped):
                continue
            statement_line = line_number
        statement = compact(f"{statement} {stripped}")
        if ";" not in stripped:
            continue
        match = DECL_RE.match(statement)
        statement = ""
        if not match:
            continue
        current_return, name, parameters = match.groups()
        current_return = compact(current_return)
        if name == "operator":
            continue
        record: dict[str, object] = {
            "source": str(path.relative_to(ROOT)),
            "scope": public_scope,
            "line": statement_line,
            "name": name,
            "signature": f"{current_return} {public_scope}::{name}({compact(parameters)}) const",
            "current_return": current_return,
        }
        record.update(classify_generated(name, current_return))
        records.append(record)
    return records


def manual_record(name: str, signature: str, current: str, target: str,
                  dtype: str, contract: str, migration: str,
                  source: str = "src/CodeN/core/NoteBase.h",
                  scope: str = "Note") -> dict[str, object]:
    return {
        "source": source,
        "scope": scope,
        "name": name,
        "signature": signature,
        "current_return": current,
        "target_return": target,
        "target_dtype": dtype,
        "contract": contract,
        "migration": migration,
    }


def manual_records() -> list[dict[str, object]]:
    rows = [
        manual_record("undefined", "static Note Note::undefined() noexcept", "Note", "Note", "untyped", "undefined handle constructor", "none", "src/CodeN/core/Note.h"),
        manual_record("nlp", "Note::NlpAccessor Note::nlp() const noexcept", "Note::NlpAccessor", "Note::NlpAccessor", "accessor", "borrowed linguistic operation gateway", "complete", "src/CodeN/core/Note.h"),
        manual_record("regex", "Note::RegexAccessor Note::regex() const noexcept", "Note::RegexAccessor", "Note::RegexAccessor", "accessor", "borrowed regular-expression operation gateway", "complete", "src/CodeN/core/Note.h"),
        manual_record("tex", "Note::TexAccessor Note::tex() const noexcept", "Note::TexAccessor", "Note::TexAccessor", "accessor", "borrowed TeX/LaTeX-source operation gateway", "complete", "src/CodeN/core/Note.h"),
        manual_record("defined", "bool Note::defined() const", "bool", "bool", "metadata", "presence query", "none"),
        manual_record("dim", "std::int64_t Note::dim() const", "std::int64_t", "std::int64_t", "metadata", "rank query", "none"),
        manual_record("size", "std::int64_t Note::size(std::int64_t dim) const", "std::int64_t", "std::int64_t", "metadata", "single extent query", "none"),
        manual_record("numel", "std::int64_t Note::numel() const", "std::int64_t", "std::int64_t", "metadata", "cardinality query", "none"),
        manual_record("empty", "bool Note::empty() const", "bool", "bool", "metadata", "zero-cardinality query", "none"),
        manual_record("is_scalar", "bool Note::is_scalar() const", "bool", "bool", "metadata", "rank query", "none"),
        manual_record("is_vector", "bool Note::is_vector() const", "bool", "bool", "metadata", "rank query", "none"),
        manual_record("is_matrix", "bool Note::is_matrix() const", "bool", "bool", "metadata", "rank query", "none"),
        manual_record("is_rank3", "bool Note::is_rank3() const", "bool", "bool", "metadata", "rank query", "none"),
        manual_record("dtype", "NoteDType Note::dtype() const", "NoteDType", "NoteDType", "metadata", "homogeneous dtype query", "none"),
        manual_record("is_text", "bool Note::is_text() const noexcept", "bool", "bool", "metadata", "homogeneous Text dtype query", "none"),
        manual_record("is_bytes", "bool Note::is_bytes() const noexcept", "bool", "bool", "metadata", "homogeneous Bytes dtype query", "none"),
        manual_record("is_bool", "bool Note::is_bool() const noexcept", "bool", "bool", "metadata", "homogeneous Bool dtype query", "none"),
        manual_record("is_int64", "bool Note::is_int64() const noexcept", "bool", "bool", "metadata", "homogeneous Int64 dtype query", "none"),
        manual_record("is_float64", "bool Note::is_float64() const noexcept", "bool", "bool", "metadata", "homogeneous Float64 dtype query", "none"),
        manual_record("sizes", "std::vector<std::int64_t> Note::sizes() const", "std::vector<std::int64_t>", "std::vector<std::int64_t>", "host metadata", "owning shape extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("sizes_view", "std::span<const std::int64_t> Note::sizes_view() const", "std::span<const std::int64_t>", "std::span<const std::int64_t>", "host metadata", "borrowed shape extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("at", "std::string_view Note::at(std::int64_t linear) const", "std::string_view", "std::string_view", "Text/Bytes", "borrowed cell extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("at_indices", "std::string_view Note::at_indices(std::span<const std::int64_t>) const", "std::string_view", "std::string_view", "Text/Bytes", "borrowed cell extraction", "none"),
        manual_record("cell", "TextRef Note::cell(std::int64_t linear) const", "TextRef", "TextRef", "Text/Bytes", "owning cell extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("cell_at", "TextRef Note::cell_at(std::span<const std::int64_t>) const", "TextRef", "TextRef", "Text/Bytes", "owning cell extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("owned", "TextRef Note::owned() const", "TextRef", "TextRef", "Text/Bytes", "owning scalar extraction", "none", "src/CodeN/core/Note.h"),
        manual_record("scalar", "std::string_view Note::scalar() const", "std::string_view", "std::string_view", "Text/Bytes", "borrowed scalar extraction; typed access for other dtypes is bool_item()/int_item()/float_item()", "none"),
        manual_record("vector", "std::vector<std::string_view> Note::vector() const", "std::vector<std::string_view>", "std::vector<std::string_view>", "Text/Bytes", "explicit borrowed host extraction", "none"),
        manual_record("matrix", "std::vector<Note> Note::matrix() const", "std::vector<Note>", "std::vector<Note>", "preserve", "row decomposition with shared storage", "complete"),
        manual_record("slabs", "std::vector<Note> Note::slabs() const", "std::vector<Note>", "std::vector<Note>", "preserve", "slab decomposition with shared storage", "complete"),
        manual_record("bool_item", "bool Note::bool_item() const", "bool", "bool", "explicit host Bool", "checked scalar Bool extraction", "complete"),
        manual_record("int_item", "std::int64_t Note::int_item() const", "std::int64_t", "std::int64_t", "explicit host Int64", "checked scalar Int64 extraction", "complete"),
        manual_record("float_item", "double Note::float_item() const", "double", "double", "explicit host Float64", "checked scalar Float64 extraction", "complete"),
        manual_record("bools", "std::span<const std::uint8_t> Note::bools() const", "std::span<const std::uint8_t>", "std::span<const std::uint8_t>", "explicit host Bool", "borrowed contiguous Bool extraction", "complete"),
        manual_record("integers", "std::span<const std::int64_t> Note::integers() const", "std::span<const std::int64_t>", "std::span<const std::int64_t>", "explicit host Int64", "borrowed contiguous Int64 extraction", "complete"),
        manual_record("reals", "std::span<const double> Note::reals() const", "std::span<const double>", "std::span<const double>", "explicit host Float64", "borrowed contiguous Float64 extraction", "complete"),
        manual_record("host", "Note::HostAny Note::host() const", "Note::HostAny", "Note::HostAny", "explicit host", "owning dtype-aware host extraction", "complete"),
        manual_record("pack", "std::vector<std::byte> Note::pack() const", "std::vector<std::byte>", "std::vector<std::byte>", "explicit host", "versioned dtype/shape/value serialization", "complete", "src/CodeN/core/Note.h"),
        manual_record("unpack", "Note pluss::note::unpack(std::span<const std::byte>)", "Note", "Note", "preserve", "validated reconstruction from Note::pack()", "complete", "include/pluss/coden/note_factory.h", "pluss::note"),
        manual_record("join_flat", "std::string Note::join_flat(std::string_view separator) const", "std::string", "std::string", "explicit host", "owning flat host extraction", "none"),
        manual_record("debug", "std::string Note::debug(std::int64_t max_items) const", "std::string", "std::string", "diagnostic", "diagnostic rendering (already branches per dtype: Text/Bytes/Bool/Int64/Float64)", "complete"),
        manual_record("take", "Note Note::take(pluss::msharp::RangeView::Int indices) const", "Note", "Note", "preserve", "closed indexed selection", "none", "src/CodeN/core/Note.h"),
        manual_record("take", "Note Note::take(const Note& indices) const", "Note", "Note", "preserve", "closed indexed selection with an Int64 Note", "complete", "src/CodeN/core/Note.h"),
        manual_record("copy_flat_to", "void Note::copy_flat_to(OutIt out) const", "void", "void", "explicit host", "borrowed flat host copy; Text/Bytes only by design, fails fast on other dtypes with a pointer to bools()/integers()/reals()", "none", "src/CodeN/core/NoteBase.h"),
        manual_record("for_each_flat", "void Note::for_each_flat(Fn&& fn) const", "void", "void", "explicit host", "borrowed flat host traversal; Text/Bytes only by design, fails fast on other dtypes with a pointer to bools()/integers()/reals()", "none", "src/CodeN/core/NoteBase.h"),
        manual_record("begin", "Note::const_iterator Note::begin() const", "const_iterator", "const_iterator", "explicit host", "borrowed flat iteration; Text/Bytes only by design, fails fast on other dtypes with a pointer to bools()/integers()/reals()", "none", "src/CodeN/core/NoteBase.h"),
        manual_record("end", "Note::const_iterator Note::end() const", "const_iterator", "const_iterator", "explicit host", "borrowed flat iteration; Text/Bytes only by design, fails fast on other dtypes with a pointer to bools()/integers()/reals()", "none", "src/CodeN/core/NoteBase.h"),
        manual_record("argsort", "Note Note::argsort(bool descending, bool stable) const", "Note", "Note", "Int64", "ordering positions", "complete", "src/CodeN/core/Note.h"),
    ]
    for op in ("==", "!=", "<", "<=", ">", ">="):
        rows.append(manual_record(
            f"operator{op}", f"Note Note::operator{op}(const Note&) const",
            "Note", "Note", "Bool",
            "shape-preserving text comparison", "complete", "src/CodeN/core/Note.h"))
        rows.append(manual_record(
            f"operator{op}", f"Note Note::operator{op}(std::string_view) const",
            "Note", "Note", "Bool",
            "shape-preserving scalar text comparison", "complete", "src/CodeN/core/Note.h"))
    for op, contract in (("&", "shape-preserving Bool conjunction"),
                        ("|", "shape-preserving Bool disjunction"),
                        ("^", "shape-preserving Bool exclusive-or")):
        rows.append(manual_record(
            f"operator{op}", f"Note Note::operator{op}(const Note&) const",
            "Note", "Note", "Bool", contract, "complete", "src/CodeN/core/Note.h"))
    rows.append(manual_record(
        "operator~", "Note Note::operator~() const",
        "Note", "Note", "Bool", "shape-preserving Bool negation", "complete",
        "src/CodeN/core/Note.h"))
    rows.append(manual_record(
        "any", "Note Note::any() const",
        "Note", "Note", "Bool", "rank-zero Bool reduction (logical or)", "complete",
        "src/CodeN/core/Note.h"))
    rows.append(manual_record(
        "all", "Note Note::all() const",
        "Note", "Note", "Bool", "rank-zero Bool reduction (logical and)", "complete",
        "src/CodeN/core/Note.h"))
    for name, signature in (
        ("operator+", "Note Note::operator+(const Note&) const"),
        ("operator+", "Note Note::operator+(std::string_view) const"),
        ("operator+=", "Note& Note::operator+=(const Note&)"),
        ("operator+=", "Note& Note::operator+=(std::string_view)"),
        ("operator*", "Note Note::operator*(std::int64_t) const"),
        ("operator*=", "Note& Note::operator*=(std::int64_t)"),
    ):
        rows.append(manual_record(name, signature, signature.split()[0], signature.split()[0],
                                  "Text", "closed text operator", "none", "src/CodeN/core/Note.h"))
    for name, signature in (
        ("operator+", "Note operator+(std::string_view prefix, const Note& rhs)"),
        ("operator*", "Note operator*(std::int64_t times, const Note& rhs)"),
    ):
        rows.append(manual_record(name, signature, "Note", "Note", "Text",
                                  "closed nonmember text operator", "none",
                                  "src/CodeN/core/Note.h", "nonmember"))
    for op in ("==", "!=", "<", "<=", ">", ">="):
        rows.append(manual_record(
            f"operator{op}",
            f"Note operator{op}(std::string_view lhs, const Note& rhs)",
            "Note", "Note", "Bool",
            "shape-preserving nonmember scalar text comparison", "complete",
            "src/CodeN/core/Note.h", "nonmember"))
    rows.append(manual_record(
        "operator<<", "std::ostream& operator<<(std::ostream& os, const Note& value)",
        "std::ostream&", "std::ostream&", "explicit host",
        "stream rendering (delegates to to_string()/render_note(), already dtype-aware)", "complete",
        "src/CodeN/core/Printing.h", "nonmember"))
    rows.extend([
        manual_record("defined", "bool TokenBatch::defined() const noexcept", "bool", "bool", "metadata", "ragged handle presence query", "none", "src/CodeN/core/TokenBatch.h", "TokenBatch"),
        manual_record("empty", "bool TokenBatch::empty() const noexcept", "bool", "bool", "metadata", "ragged result emptiness query", "none", "src/CodeN/core/TokenBatch.h", "TokenBatch"),
        manual_record("tokens", "const Note& TokenBatch::tokens() const", "const Note&", "const Note&", "Text", "packed ragged values", "none", "src/CodeN/core/TokenBatch.h", "TokenBatch"),
        manual_record("offsets", "const Note& TokenBatch::offsets() const", "const Note&", "const Note&", "Int64", "packed ragged segment offsets", "complete", "src/CodeN/core/TokenBatch.h", "TokenBatch"),
    ])
    return rows


def build_manifest() -> dict[str, object]:
    records = (
        generated_records()
        + accessor_records(NLP_DECLS, "NoteNlpAccessor", "Note::NlpAccessor")
        + accessor_records(REGEX_DECLS, "NoteRegexAccessor", "Note::RegexAccessor")
        + accessor_records(TEX_DECLS, "NoteTexAccessor", "Note::TexAccessor")
        + manual_records()
    )
    summary: dict[str, int] = {}
    for record in records:
        key = f"{record['target_return']}:{record['target_dtype']}"
        summary[key] = summary.get(key, 0) + 1
    return {
        "schema_version": 1,
        "policy": {
            "priority": ["accuracy", "vectorization_and_batching", "efficiency"],
            "dtypes": ["Text", "Bytes", "Bool", "Int64", "Float64"],
            "bool_storage": "contiguous uint8 lanes; never std::vector<bool>",
            "numeric_storage": "aligned contiguous native lanes",
            "text_storage": "packed arena plus spans",
            "ragged_storage": "TokenBatch containing Note tokens and Int64 Note offsets",
        },
        "summary": dict(sorted(summary.items())),
        "records": records,
    }


def validate_removed_escapes() -> None:
    for path in PUBLIC_IMPLEMENTATION_FILES:
        source = path.read_text(encoding="utf-8")
        for name in FORBIDDEN_ESCAPE_NAMES:
            if name in source:
                raise RuntimeError(
                    f"removed cross-type escape {name} reappeared in {path.relative_to(ROOT)}"
                )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    validate_removed_escapes()
    manifest = build_manifest()
    rendered = json.dumps(manifest, indent=2, ensure_ascii=True) + "\n"
    if args.write:
        args.output.write_text(rendered, encoding="utf-8")
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != rendered:
            raise SystemExit("closed-Note contract manifest is stale; run with --write")
    print(f"audited {len(manifest['records'])} public signatures")
    for key, count in manifest["summary"].items():
        print(f"{key}: {count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
