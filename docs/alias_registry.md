# Alias Registry And Naming Policy

This file is the single source of truth for short public aliases.
Machine-readable source for CI lives in `docs/alias_registry.yml`.

## Validation

Run local validation:

```bash
python3 tools/lint_alias_registry.py
```

Or via CMake target:

```bash
cmake --build build --target alias_lint
```

Or via CTest:

```bash
ctest --test-dir build -R alias_registry_lint --output-on-failure
```

CI also runs this validation on push/PR.

## Why

1. Keep call sites concise.
2. Prevent name clashes as aliases grow.
3. Keep API discoverable and type-safe.

## Scope

This policy applies codebase-wide (`PlussPl`, `PlussMic`, `PlussMa`, `PlussQit`, `PlussView`).

## Hard Rules

1. No method/function aliases in public API.
Only one canonical method name is allowed per behavior.

2. Aliases are for enum values and compact option access only.
Prefer typed constants over string names.

3. Alias namespace names are lowercase and at most 4 letters.
Examples: `ks`, `fs`, `fc`, `fp`.

4. Aliases must be module-scoped, not global.
Define aliases inside the owning namespace (for example `pluss::pl`, `pluss::pl::flash`).

5. Do not use `using namespace` in headers to expose aliases.
Keep explicit namespace access to avoid hidden collisions.

6. Every new alias namespace must be registered in this file before merge.

## Collision Strategy

1. Prefer 2-letter names first, then 3-4 letters only when needed.
2. Reuse existing mnemonic pattern:
- `k*` for key/legend related enums in `pluss::pl`
- `f*` for flash enums in `pluss::pl::flash`
3. If a same-scope clash appears, resolve by:
- keeping existing alias untouched,
- renaming only the new alias to a longer mnemonic within the 4-letter limit.

## Approved Aliases

### `pluss::subsp::stat::distr::dst::Kind`

- `dst::Kind`
  - Distribution kind shorthand namespace (see header for full value list).
  - Defined in: `PlussMa/include/pluss/subsp/stat/distr_fwd.h`
  - Local opt-in alias header: `PlussMa/include/pluss/subsp/stat/dst_alias.h`
    - Exposes `pluss::ma::dst::Normal` (module-scoped alias, lower collision risk).
  - Library-wide alias umbrella: `PlussMa/include/pluss/ma/aliases.h`
    - Exposes `ma::dst::Normal` (adds `namespace ma = ::pluss::ma`).
  - Global alias umbrella: `PlussQit/include/pluss/aliases.h`
    - Includes available library alias umbrellas (currently PlussMa).

### `pluss::pl`

- `ks` -> `KeyShape`
  - `ks::Auto`
  - `ks::Horizontal`
  - `ks::Vertical`
  - Defined in: `PlussPl/include/pluss/pl/studio.h`

### `pluss::pl::flash`

- `fc` -> `FlashContext`
  - `fc::Inherit`, `fc::Paper`, `fc::Notebook`, `fc::Talk`, `fc::Poster`
- `fs` -> `FlashStyle`
  - `fs::Darkgrid`, `fs::Lightgrid`, `fs::Sandgrid`, `fs::Dark`, `fs::Light`, `fs::Sand`
- `fp` -> `FlashPalette`
  - `fp::Deep`, `fp::Muted`, `fp::Pastel`, `fp::Bright`, `fp::Dark`, `fp::Colorblind`
  - Defined in: `PlussPl/include/pluss/pl/flash/detail/flash_style.h`

## Change Process

1. Propose alias namespace + enum mapping.
2. Add it to this registry and `docs/alias_registry.yml`.
3. Run `python3 tools/lint_alias_registry.py`.
4. Add at least one example usage in a demo or docs file.

## User Alias Cookbook

Official aliases are maintainer-managed and intentionally small.  
Project-specific aliases should be user-managed in app code.

Recommended pattern:

1. Keep aliases in one local header (for example `project_aliases.h`).
2. Prefer module-qualified aliases (`ma::...`, `qit::...`, `pl::...`).
3. Alias namespaces/types, not functions/methods.
4. Keep names short but semantic (`dist`, `opt`, `tbl`), avoid single-letter aliases.
5. Do not put `using namespace ...` in shared headers.

Example (`project_aliases.h`):

```cpp
#pragma once

#include <pluss/aliases.h> // optional Pluss umbrella aliases

namespace pm = ::pluss::ma;
namespace pq = ::pluss::subsp::math::optim;
namespace pp = ::pluss::pl;

// PlussMa distribution kinds
namespace dist = ::pluss::subsp::stat::distr::dst;
```

Usage:

```cpp
auto x = pluss::distr::sample(pm::dst::Normal, {0.0, 1.0}, {1024}, cfg, 42);
auto y = pluss::distr::sample(dist::Normal, {0.0, 1.0}, {1024}, cfg, 42);
```

Anti-patterns:

1. Defining global aliases in third-party/public headers.
2. Re-aliasing the same concept with many names in one project.
3. Alias schemes that hide module ownership.

## Non-Goals

1. This does not replace canonical API names.
2. This does not permit duplicate method names or behavior aliases.
