#!/usr/bin/env python3
"""Generates _static/api_nav.js — the data manifest that drives PlussPl's
persistent, clickable per-class API sidebar (see _static/layout.js's
buildApiMethodSidebar()/currentApiPage()), mirroring the mechanism already
used by PlussMic (see PlussMic/docs/sphinx/generate_api_reference.py's
write_api_nav_manifest()).

PlussPl has no C++-parsing doc generator of its own — every page under
api/{pane,studio,trace,flash}/ was hand-authored directly as markdown. So
unlike PlussMic's manifest (a byproduct of parsing headers), this script's
only source of truth is the already-written page files on disk, plus (for
flash only) a hardcoded family map that mirrors flash_api_reference.md's own
toctree order/grouping.

Run this whenever a page is added, removed, or renamed under
api/{pane,studio,trace,flash}/, then rebuild the site.
"""

from __future__ import annotations

import json
from pathlib import Path

SPHINX_ROOT = Path(__file__).resolve().parent
STATIC_DIR = SPHINX_ROOT / "_static"

# ---------------------------------------------------------------------------
# Pane / Studio / Trace: plain alphabetical classes, exactly like every
# PlussMic class (DataFrame, Catalog, Slate, ...) — the sidebar groups by
# first letter, built straight from the files on disk.
# ---------------------------------------------------------------------------

ALPHABETICAL_CLASSES = {
    "Pane": {"slug": "pane", "landing": "pane_api_reference.html"},
    "Studio": {"slug": "studio", "landing": "studio_api_reference.html"},
    "Trace": {"slug": "trace", "landing": "trace_api_reference.html"},
    "Types": {"slug": "types", "landing": "types_api_reference.html"},
}

# ---------------------------------------------------------------------------
# flash: grouped by conceptual family instead of by letter. This mirrors
# flash_api_reference.md's own toctree order/grouping exactly (see that
# page's "Coverage" section) rather than re-deriving it from anything
# structural, since the family boundaries are an authorial/API-design
# decision, not something derivable from the files themselves.
# ---------------------------------------------------------------------------

FLASH_FAMILIES = [
    {"title": "Relational", "methods": ["scatterplot", "lineplot", "stepplot", "relplot"]},
    {"title": "Distribution", "methods": ["histplot", "displot", "kdeplot", "ecdfplot", "areaplot", "rugplot"]},
    {"title": "Categorical", "methods": [
        "stripplot", "swarmplot", "boxplot", "violinplot", "boxenplot",
        "pointplot", "whiskplot", "barplot", "countplot", "pieplot", "catplot",
    ]},
    {"title": "Regression and Residual Diagnostics", "methods": ["regplot", "lmplot", "residplot", "qqplot", "resid_histplot"]},
    {"title": "Matrix", "methods": ["heatmap", "clustermap"]},
    {"title": "Grid Constructors and Fill Helpers", "methods": [
        "FacetGrid", "PairGrid", "JointGrid", "relgridplot", "catgridplot", "disgridplot",
        "facet_map_plot", "pairgrid_map_plot", "jointgrid_map_plot",
    ]},
    {"title": "Multivariate Grid Views", "methods": ["pairplot", "triplot"]},
    {"title": "Multivariate Curve and Glyph Views", "methods": ["parallelplot", "andrews_curves", "radviz"]},
    {"title": "Jointplot", "methods": ["jointplot"]},
    {"title": "Summaryplot", "methods": ["summaryplot"]},
    {"title": "Time-Series Diagnostics", "methods": [
        "lagplot", "acfplot", "pacfplot", "ccfplot", "actplot", "resid_acfplot", "bootstrapplot",
    ]},
    {"title": "3D Native", "methods": ["path3", "cloud3", "sheet", "lattice", "spire3", "ridge3", "isoshell"]},
    {"title": "3D Convenience Aliases", "methods": [
        "plot3D", "scatter3D", "plot_surface", "plot_wireframe", "contour3D", "bar3d", "hist3d",
    ]},
    {"title": "3D and 4D Scene, Camera, Lighting, Animation", "methods": [
        "camera3d", "camera_preset3d", "lighting3d", "theme_preset3d", "scene_preset3d",
        "scene3d", "trace_preset3d", "trace3d", "path4", "cloud4",
        "hyper_animation_sequence", "animate3", "animate4", "export4_frames",
    ]},
    {"title": "Theming and Context", "methods": ["set_theme", "set_style", "set_context"]},
    {"title": "Legend, Grid, Layout, Export", "methods": [
        "move_legend", "legend_frame", "tight_layout", "despine", "tickless",
        "horizontal_grid", "show", "save_png",
    ]},
    {"title": "Palette Generation", "methods": [
        "flash_palette", "color_palette", "flash_theme", "blend_palette", "hls_palette",
        "husl_palette", "cubehelix_palette", "light_palette", "dark_palette",
        "diverging_palette", "mpl_palette", "xkcd_palette", "crayon_palette",
    ]},
    {
        "title": "Bundled Datasets",
        "methods": ["get_dataset_names", "load_dataset"],
        "subgroups": [
            {
                "title": "Dataset",
                "methods": [
                    "Dataset",
                    "dataset_from_table",
                    "dataset_frame",
                    "dataset_nrows",
                    "dataset_ncols",
                    "dataset_size",
                ],
            },
        ],
    },
]


def page_names(slug: str) -> list[str]:
    directory = SPHINX_ROOT / "api" / slug
    names = sorted(p.stem for p in directory.glob("*.md"))
    return names


def build_alphabetical_entry(title: str, slug: str, landing: str) -> dict:
    methods = page_names(slug)
    return {"title": title, "slug": slug, "landing": landing, "methods": methods}


def build_flash_entry() -> dict:
    on_disk = set(page_names("flash"))
    grouped: set[str] = set()
    groups = []
    for family in FLASH_FAMILIES:
        family_title = family["title"]
        names = family.get("methods", [])
        present = [n for n in names if n in on_disk]
        for n in names:
            if n not in on_disk:
                raise SystemExit(
                    f"generate_api_nav: FLASH_FAMILIES lists '{n}' (family '{family_title}') "
                    f"but api/flash/{n}.md does not exist on disk. Fix the family map."
                )
        grouped.update(present)
        group_entry = {"title": family_title, "methods": present}

        subgroups = []
        for subgroup in family.get("subgroups", []):
            subgroup_title = subgroup["title"]
            subgroup_names = subgroup.get("methods", [])
            subgroup_present = [n for n in subgroup_names if n in on_disk]
            for n in subgroup_names:
                if n not in on_disk:
                    raise SystemExit(
                        f"generate_api_nav: FLASH_FAMILIES subgroup '{subgroup_title}' "
                        f"(family '{family_title}') lists '{n}' but api/flash/{n}.md does not exist on disk."
                    )
            grouped.update(subgroup_present)
            subgroups.append({"title": subgroup_title, "methods": subgroup_present})
        if subgroups:
            group_entry["subgroups"] = subgroups
        groups.append(group_entry)

    missing = sorted(on_disk - grouped)
    if missing:
        raise SystemExit(
            "generate_api_nav: these api/flash/*.md pages exist on disk but aren't listed in "
            f"any FLASH_FAMILIES group: {missing}. Add them to the appropriate family."
        )

    return {
        "title": "flash",
        "slug": "flash",
        "landing": "flash_api_reference.html",
        "groups": groups,
    }


def main() -> None:
    nav = {}
    for title, cfg in ALPHABETICAL_CLASSES.items():
        nav[title] = build_alphabetical_entry(title, cfg["slug"], cfg["landing"])
    nav["flash"] = build_flash_entry()

    STATIC_DIR.mkdir(parents=True, exist_ok=True)
    out_path = STATIC_DIR / "api_nav.js"
    body = json.dumps(nav, indent=2, ensure_ascii=False)
    out_path.write_text(f"window.PLUSS_API_NAV = {body};\n", encoding="utf-8")

    def flash_group_count(group: dict) -> int:
        return len(group.get("methods", [])) + sum(
            len(subgroup.get("methods", []))
            for subgroup in group.get("subgroups", [])
        )

    counts = ", ".join(
        f"{k}={len(v['methods'])}"
        if "methods" in v
        else f"{k}={sum(flash_group_count(g) for g in v['groups'])}"
        for k, v in nav.items()
    )
    print(f"[generate_api_nav] wrote {out_path} ({counts})")


if __name__ == "__main__":
    main()
