#!/usr/bin/env python3
"""Unified Pluss documentation build orchestrator.

This is the single supported docs build graph for the repo. Package-local
wrappers and the root docs hub should delegate here instead of carrying
separate aggregation, generation, or Sphinx logic.

The default `html` path rebuilds every package docs hub and the root docs hub
so one command leaves every browsable HTML output in sync.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Sequence


REPO_ROOT = Path(__file__).resolve().parents[2]
PYTHON = sys.executable
AGGREGATOR = REPO_ROOT / "docs/scripts/aggregate_docs.py"
DOC_EXAMPLE_VALIDATOR = REPO_ROOT / "docs/scripts/validate_doc_examples.py"


@dataclass(frozen=True)
class GeneratorStep:
    script: Path
    supports_signature_check: bool = False


@dataclass(frozen=True)
class Site:
    key: str
    title: str
    docs_dir: Path
    scan_root: Path | None = None
    hub_slug: str | None = None
    generators: tuple[GeneratorStep, ...] = field(default_factory=tuple)

    @property
    def build_dir(self) -> Path:
        return self.docs_dir / "_build"


SITES: dict[str, Site] = {
    "plussqit": Site(
        key="plussqit",
        title="PlussQit",
        docs_dir=REPO_ROOT / "PlussQit/docs",
        scan_root=REPO_ROOT / "PlussQit",
        hub_slug="PlussQit",
    ),
    "plussmic": Site(
        key="plussmic",
        title="PlussMic",
        docs_dir=REPO_ROOT / "PlussMic/docs/sphinx",
        scan_root=REPO_ROOT / "PlussMic",
        hub_slug="PlussMic",
        generators=(
            GeneratorStep(
                script=REPO_ROOT / "PlussMic/docs/sphinx/generate_api_reference.py",
                supports_signature_check=True,
            ),
        ),
    ),
    "plussma": Site(
        key="plussma",
        title="PlussMa",
        docs_dir=REPO_ROOT / "PlussMa/docs",
        scan_root=REPO_ROOT / "PlussMa",
        hub_slug="PlussMa",
    ),
    "plusspl": Site(
        key="plusspl",
        title="PlussPl",
        docs_dir=REPO_ROOT / "PlussPl/docs/sphinx",
        scan_root=REPO_ROOT / "PlussPl",
        hub_slug="PlussPl",
    ),
    "plussai": Site(
        key="plussai",
        title="PlussAI",
        docs_dir=REPO_ROOT / "PlussAI/docs/sphinx",
        scan_root=REPO_ROOT / "PlussAI",
        hub_slug="PlussAI",
        generators=(
            GeneratorStep(
                script=REPO_ROOT / "PlussAI/docs/sphinx/generate_nodej_lymph_api_docs.py",
            ),
        ),
    ),
    "plusstils": Site(
        key="plusstils",
        title="PlussTils",
        docs_dir=REPO_ROOT / "PlussTils/view/docs",
        scan_root=REPO_ROOT / "PlussTils",
        hub_slug="PlussTils",
    ),
    "root": Site(
        key="root",
        title="Root Docs",
        docs_dir=REPO_ROOT / "docs/sphinx",
    ),
}

PACKAGE_BUILD_ORDER = [
    SITES["plussqit"],
    SITES["plussmic"],
    SITES["plussma"],
    SITES["plusspl"],
    SITES["plussai"],
    SITES["plusstils"],
]
ROOT_SITE = SITES["root"]


def run(cmd: Sequence[str], cwd: Path | None = None) -> None:
    where = cwd if cwd is not None else REPO_ROOT
    print(f"[docs] ({where}) $ {' '.join(str(part) for part in cmd)}", flush=True)
    subprocess.run(list(map(str, cmd)), cwd=where, check=True)


def resolve_sphinx_build() -> str:
    explicit = os.environ.get("SPHINXBUILD")
    if explicit:
        return explicit

    for candidate in (
        shutil.which("sphinx-build"),
        str(REPO_ROOT / ".venv_docs/bin/sphinx-build"),
    ):
        if candidate and Path(candidate).exists():
            return candidate

    raise SystemExit(
        "sphinx-build not found. Set SPHINXBUILD, activate the docs environment, "
        "or install docs/sphinx/requirements.txt."
    )


def resolve_latexmk() -> str:
    candidate = shutil.which("latexmk")
    if candidate:
        return candidate
    raise SystemExit("latexmk not found. Install TeX tooling before running the pdf target.")


def run_generators(signature_check: bool) -> None:
    for site in PACKAGE_BUILD_ORDER:
        for step in site.generators:
            cmd = [PYTHON, str(step.script)]
            if signature_check and step.supports_signature_check:
                cmd.append("--check")
            run(cmd, cwd=REPO_ROOT)


def aggregate_package_sites() -> None:
    for site in PACKAGE_BUILD_ORDER:
        if site.scan_root is None:
            continue
        run(
            [
                PYTHON,
                str(AGGREGATOR),
                "--into",
                str(site.docs_dir),
                "--scan",
                str(site.scan_root),
            ],
            cwd=REPO_ROOT,
        )


def aggregate_root_site() -> None:
    cmd = [PYTHON, str(AGGREGATOR), "--into", str(ROOT_SITE.docs_dir)]
    for site in PACKAGE_BUILD_ORDER:
        if site.hub_slug is None:
            continue
        cmd.extend(["--package-hub", f"{site.hub_slug}={site.docs_dir}"])
    run(cmd, cwd=REPO_ROOT)


def prepare(signature_check: bool) -> None:
    print("[docs] Preparing unified docs sources...", flush=True)
    run_generators(signature_check=signature_check)
    aggregate_package_sites()
    aggregate_root_site()
    run(
        [
            PYTHON,
            str(DOC_EXAMPLE_VALIDATOR),
            str(REPO_ROOT / "PlussQit/docs/sphinx/api/series"),
        ],
        cwd=REPO_ROOT,
    )


def sphinx_cmd(sphinxbuild: str, builder: str, site: Site, strict: bool) -> list[str]:
    cmd = [sphinxbuild, "-E"]
    if strict:
        cmd.extend(["-W", "--keep-going"])
    cmd.extend(["-b", builder, str(site.docs_dir), str(site.build_dir / ("html" if builder == "html" else "latex"))])
    return cmd


def build_html(site: Site, sphinxbuild: str, strict: bool) -> None:
    run(sphinx_cmd(sphinxbuild, "html", site, strict), cwd=REPO_ROOT)


def build_latex(site: Site, sphinxbuild: str, strict: bool) -> None:
    run(sphinx_cmd(sphinxbuild, "latex", site, strict), cwd=REPO_ROOT)


def build_latexpdf(site: Site, sphinxbuild: str, strict: bool) -> None:
    cmd = [sphinxbuild, "-E"]
    if strict:
        cmd.extend(["-W", "--keep-going"])
    cmd.extend(["-b", "latexpdf", str(site.docs_dir), str(site.build_dir / "latex")])
    run(cmd, cwd=REPO_ROOT)


def build_pdf(site: Site, sphinxbuild: str, strict: bool) -> None:
    build_latex(site=site, sphinxbuild=sphinxbuild, strict=strict)
    latexmk = resolve_latexmk()
    tex_files = sorted((site.build_dir / "latex").glob("*.tex"))
    if not tex_files:
        raise SystemExit(f"No .tex files found under {site.build_dir / 'latex'} for {site.title}.")
    run([latexmk, "-pdf", "-quiet", *[tex.name for tex in tex_files]], cwd=site.build_dir / "latex")


def clean_sites(sites: list[Site]) -> None:
    for site in sites:
        if site.build_dir.exists():
            print(f"[docs] removing {site.build_dir}", flush=True)
            shutil.rmtree(site.build_dir)


def resolve_sites(selection: str) -> list[Site]:
    if selection == "all":
        return [*PACKAGE_BUILD_ORDER, ROOT_SITE]
    if selection == "packages":
        return list(PACKAGE_BUILD_ORDER)
    if selection == "root":
        return [ROOT_SITE]
    if selection not in SITES:
        raise SystemExit(f"unknown site selection: {selection}")
    return [SITES[selection]]


def default_site_for_action(action: str) -> str:
    if action in {"aggregate", "html", "clean", "prepare"}:
        return "all"
    return "root"


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the unified Pluss docs graph.")
    parser.add_argument(
        "action",
        choices=("aggregate", "prepare", "html", "latex", "latexpdf", "pdf", "clean"),
        help="build action to run",
    )
    parser.add_argument(
        "--site",
        choices=("all", "packages", *SITES.keys()),
        help="which docs hub(s) to build or clean",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="pass -W --keep-going to sphinx-build",
    )
    parser.add_argument(
        "--signature-check",
        action="store_true",
        help="run PlussMic API generation in signature-drift check mode",
    )
    parser.add_argument(
        "--skip-prepare",
        action="store_true",
        help="skip generators and aggregation before build actions",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if args.action in {"aggregate", "prepare"}:
        prepare(signature_check=args.signature_check)
        return 0

    selection = args.site or default_site_for_action(args.action)
    sites = resolve_sites(selection)

    if args.action == "clean":
        clean_sites(sites)
        return 0

    if not args.skip_prepare:
        prepare(signature_check=args.signature_check)

    sphinxbuild = resolve_sphinx_build()
    if args.action == "html":
        for site in sites:
            build_html(site=site, sphinxbuild=sphinxbuild, strict=args.strict)
        return 0
    if args.action == "latex":
        for site in sites:
            build_latex(site=site, sphinxbuild=sphinxbuild, strict=args.strict)
        return 0
    if args.action == "latexpdf":
        for site in sites:
            build_latexpdf(site=site, sphinxbuild=sphinxbuild, strict=args.strict)
        return 0
    if args.action == "pdf":
        for site in sites:
            build_pdf(site=site, sphinxbuild=sphinxbuild, strict=args.strict)
        return 0

    raise SystemExit(f"unhandled action: {args.action}")


if __name__ == "__main__":
    raise SystemExit(main())
