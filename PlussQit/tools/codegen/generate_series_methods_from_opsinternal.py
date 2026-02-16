#!/usr/bin/env python3
"""Generate Series method wrappers from CodeM/core/OpsInternal.h.

This repository variant keeps CodeM generation lightweight; for now, we emit
empty wrapper includes so the library builds cleanly even when CodeM's
OpsInternal surface is evolving.

If/when you want full Series surface synthesis, replace this script with the
real extractor.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--opsinternal", required=True)
    ap.add_argument("--series-header", required=True)
    ap.add_argument("--out-decls", required=True)
    ap.add_argument("--out-defs", required=True)
    args = ap.parse_args()

    out_decls = Path(args.out_decls)
    out_defs = Path(args.out_defs)
    out_decls.parent.mkdir(parents=True, exist_ok=True)

    out_decls.write_text(
        "// AUTO-GENERATED (placeholder)\n"
        "// Series wrappers from OpsInternal are not generated in this patch.\n\n",
        encoding="utf-8",
    )
    out_defs.write_text(
        "// AUTO-GENERATED (placeholder)\n"
        "// Series wrapper definitions from OpsInternal are not generated in this patch.\n\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
