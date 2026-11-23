"""Which documentation pages are public and what their public text is.

The rules mirror the website's page sync (scripts/sync-docs.py in the website repository): the same
files, the same addresses and the same wording changes, so the pages that doc() prints are the pages
the website shows. Keep the two in step.
"""
import re
from pathlib import Path

BLOCKED = {"_build", "_local", ".doctrees", "node_modules"}
INTERNAL = {
    "README.md", "build_outputs.md", "plussmic_docs_hub.md", "Series_Doc_Authoring_Standard.md",
    "API_Doc_Authoring_Standard.md", "CodeM_MethodRoutingPlan.md", "derivative_coverage.md", "PATCHES.md",
    "USER_GUIDE_STANDARD.md", "dl_torch_maturity_blueprint.md", "autodiff_architecture.md",
    "optimization_diff_contract.md", "plusspl_blueprint.md", "dataframe_benchmarks.md",
}


def roots(source):
    yield "guide", source / "docs" / "sphinx"
    for name, folder in (("qit", "PlussQit"), ("mic", "PlussMic"), ("ma", "PlussMa"), ("pl", "PlussPl"), ("ai", "PlussAI"), ("tils", "PlussTils")):
        yield name, source / folder / "docs"


def slug_for(path, space, folder):
    relative = path.relative_to(folder)
    if space != "guide" and relative.parts[0] == "sphinx":
        relative = Path(*relative.parts[1:])
    return f"{space}/{relative.with_suffix('').as_posix()}"


def public_pages(source):
    """Yield (space, folder, path) for every public page."""
    for space, folder in roots(source):
        if not folder.is_dir():
            continue
        for path in sorted(folder.rglob("*.md")):
            relative = path.relative_to(folder)
            if any(part in BLOCKED for part in relative.parts) or path.is_symlink():
                continue
            if path.name in INTERNAL or (path.name.startswith("optima_") and "parity" in path.stem.lower()) or path.name.endswith("_todo.md"):
                continue
            if space == "guide" and path.name == "series_api_reference.md":
                continue
            yield space, folder, path


def public_copy(content, path, source):
    content = re.sub(r"<!-- apiref:nav:start -->.*?<!-- apiref:nav:end -->\s*", "", content, flags=re.DOTALL)
    replacements = (
        (r"\bPyTorch-like\b", "eager tensor"),
        (r"\bPyTorch\b", "other tensor libraries"),
        (r"\bMATLAB-style\b", "constrained optimization"),
        (r"\bMATLAB\b", "other technical computing tools"),
        (r"\bJAX\b", "other autodiff libraries"),
        (r"\bNumPy\b", "array libraries"),
        (r"\bpandas\b", "dataframe libraries"),
    )
    for pattern, replacement in replacements:
        content = re.sub(pattern, replacement, content, flags=re.IGNORECASE)
    if path.relative_to(source).as_posix() == "PlussMic/docs/sphinx/manual.md":
        content = re.sub(r"^## Verified examples\n.*?(?=^## |\Z)", "", content, flags=re.MULTILINE | re.DOTALL)
    if path.is_relative_to(source / "PlussQit" / "docs"):
        content = re.sub(r"\]\(sphinx/([^)]*?)\.md\)", r"](/docs/qit/\1)", content)
    content = content.replace("`PlussMic/docs/sphinx/manual/examples/plussmic_manual_examples.cpp`", "the verified manual examples")
    content = re.sub(r"`docs/sphinx/_verified/[^`]+`", "the verified example output", content)
    return content


def transform(content, path, source, depth=0):
    if depth > 3:
        return content
    directive = re.compile(r"^```\{(include|toctree)\}([^\n]*)\n(.*?)^```\s*$", re.MULTILINE | re.DOTALL)

    def replace(match):
        kind, argument, body = match.groups()
        if kind == "toctree" and ":hidden:" in body:
            return ""
        if kind == "include":
            included = (path.parent / argument.strip()).resolve()
            if included.is_file() and included.is_relative_to(source):
                return transform(included.read_text(encoding="utf-8", errors="replace"), included, source, depth + 1)
            return ""
        links = []
        for line in body.splitlines():
            item = line.strip()
            if not item or item.startswith(":") or "*" in item:
                continue
            named = re.fullmatch(r"(.+?)\s*<(.+?)>", item)
            label, target = (named.group(1), named.group(2)) if named else (item.replace("_", " ").title(), item)
            if not target.endswith((".md", ".rst", ".html")):
                target += ".md"
            links.append(f"- [{label}]({target})")
        return "\n".join(links)

    return directive.sub(replace, content)


def public_text(path, source):
    content = path.read_text(encoding="utf-8", errors="replace")
    return public_copy(transform(content, path, source), path, source).rstrip() + "\n"


def title_of(content, path):
    heading = re.search(r"^#\s+(.+?)\s*$", content, re.MULTILINE)
    return re.sub(r"[`*]", "", heading.group(1)).strip() if heading else path.stem.replace("_", " ").replace("-", " ").title()
