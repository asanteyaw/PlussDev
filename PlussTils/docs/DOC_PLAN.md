# PlussTils Documentation Plan

This plan follows the repository-wide standard in
`PlussQit/docs/API_Doc_Authoring_Standard.md`.

PlussTils is a shelf of optional convenience utilities. Its documentation must
not imply that Core, Loaders, PlussView, and PlussBI form a required pipeline or
that adopting one family requires adopting the others. Cross-family recipes are
appropriate only when the APIs genuinely compose.

## Frozen public families

| Family | Canonical public namespace | Canonical header | Role |
|---|---|---|---|
| Core | `pluss::tils` | `<pluss/tils.h>` | Small language, environment, output, conversion, and dispatch conveniences |
| Loaders | `pluss::loader` | `<pluss/tils/kit/data_loader.h>` and neighboring loader headers | Batching, sampling, splitting, and filesystem-backed input support |
| PlussBI | `pluss::bi` | `<pluss/bi/bi.h>` | Declarative analytical boards, controls, views, sessions, and browser serving |
| PlussView | `pluss::view` and `pluss::view::window` | `<pluss/tils/view.h>` | Standalone window, scene, rendering, interaction, and export utilities |

The implementation namespaces `pluss::bi::detail` and any internal helper type
are not documentation surfaces.

## Canonical surface decisions

- Loader types live directly in `pluss::loader`; there is no second
  `pluss::tils` spelling.
- Numeric scalar casts use lowercase names such as `as_i64`; uppercase
  forwarding aliases are removed.
- `pluss::tils::let` and `pluss::tils::suppose` require their namespace; they
  are not injected into the global namespace.
- `map_axis` is the canonical axis-wise transformation name.
- PlussBI's free-function DSL (`from`, `category`, `line`, and peers) is the
  canonical concise construction surface. Exact static forwarding duplicates
  are removed; generic `View::make` and lower-level `BindSpec` builders remain.
- PlussView is an optional utility family, not the package root and not a
  dependency implied by the Core or Loader documentation.

## Delivery phases

1. Freeze and clean the canonical public surface.
2. Establish the package documentation hub, navigation, and family landings.
3. Document Core utilities and pathing.
4. Document loaders, samplers, and splitting.
5. Build the PlussView manual and reference.
6. Build the PlussBI manual and reference.
7. Add genuine cross-family recipes, permanent verifiers, exact outputs, and
   complete navigation checks.

Every substantive family receives a permanent executable. Displayed output is
copied from that executable, never reconstructed by hand.
