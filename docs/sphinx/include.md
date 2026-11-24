# Which header to include

PlussII is six spaces, and one header gives you the starting set. Everything else is opt-in: you include it yourself, so a program compiles only what it uses. This page walks through the includes from the single line to the single piece, says what each one gives you, and explains the short aliases.

## Three levels

You can include PlussII at three levels of detail, from the most convenient to the most precise.

1. **`<pluss/pluss.h>`**: the one to use. It gives you PlussQit, PlussMic and PlussPl, and it is what almost every program, guide and example in this documentation includes. One line, nothing to order, and no "why is this type not found" to chase. Reach for the others only when you have a reason.
2. **One header for each space**: `<pluss/core.h>`, `<pluss/mic.h>`, `<pluss/pl.h>`, `<pluss/ma.h>`, `<pluss/ai.h>`, `<pluss/tils.h>`. `<pluss/pluss.h>` is exactly the first three of these.
3. **A single header for a single piece**: `<pluss/io.h>`, `<pluss/pl/flash.h>`, `<pluss/tils/doc.h>` and the others listed below. Each one stands alone: it includes what it needs, so you can include it first, last or by itself.

`<pluss/pluss.h>` gives you PlussQit, PlussMic and PlussPl. PlussMa, PlussAI and PlussTils are not in it: add `<pluss/ma.h>`, `<pluss/ai.h>` or `<pluss/tils.h>` when you want them. Linking is separate: one CMake target, `Pluss::PlussII`, links every library, so the header you include decides what is compiled, not what is linked.

Finer includes compile faster and show exactly what a file depends on. The price is typing and remembering which header holds what, which is why the single line is the right default.

## The umbrella headers

| Header | What it gives you |
|---|---|
| `<pluss/pluss.h>` | PlussQit, PlussMic and PlussPl together: the usual starting point |
| `<pluss/core.h>` | PlussQit: `Series`, `Note`, `Timeline`, `Duration`, `TimeIndex`, `Queue`, saving and loading, errors |
| `<pluss/mic.h>` | PlussMic: the table, kindle, scribble and bundle models and the loaders (it also brings in plotting when PlussPl is present) |
| `<pluss/pl.h>` | PlussPl: `Studio`, `Pane`, `Trace`, the styles and views, and the `flash` plots |
| `<pluss/ma.h>` | PlussMa: math, stats, engineering, natural sciences and actuarial |
| `<pluss/ai.h>` | PlussAI: the neural-network, machine-learning and procedure frameworks |
| `<pluss/tils.h>` | PlussTils: every PlussTils header, which includes the dashboards (PlussBI) when PlussMic is installed |

## The single headers

### PlussQit

`<pluss/core.h>` is made of these. Take one when a file needs only that.

| Header | What it gives you |
|---|---|
| `<pluss/codem/series_factory.h>` | creating a `Series`: `make_series`, `zeros`, `where` and the other factories |
| `<pluss/coden/note_factory.h>` | creating a `Note` of text |
| `<pluss/codeh/timeline_factory.h>` | creating a `Timeline` of instants, with `Duration` and `TimeIndex` |
| `<pluss/queue.h>` | `Queue`, the one value that holds a `Series`, a `Note`, a `Timeline` or a `Duration` |
| `<pluss/pluss_type.h>` | the dynamic holder behind `Queue` |
| `<pluss/io.h>` | saving and loading values |
| `<pluss/array_file.h>` | array files (`.npy` and `.npz`) |
| `<pluss/error.h>`, `<pluss/exception.h>` | errors, tracebacks and messages |
| `<pluss/validate.h>` | checks that suggest the closest valid name when one fails |
| `<pluss/null.h>`, `<pluss/slicing.h>` | missing values, and slicing |
| `<pluss/codem/losses.h>` | loss functions on a `Series` |
| `<pluss/subsp/linearalg/linearalg.h>`, `<pluss/subsp/math/linalg.h>`, `<pluss/subsp/math/diff.h>`, `<pluss/subsp/stats/stats.h>` | linear algebra, differentiation and statistics on a `Series` |
| `<pluss/version.h>` | `pluss::version()`, the version and build of this PlussII |

### PlussMic

| Header | What it gives you |
|---|---|
| `<pluss/table/table.h>` | the table model: `DataFrame`, `Shelf`, `Atlas`, the example tables, display and plotting from a table |
| `<pluss/kindle/kindle.h>` | the kindle model: `Stream`, `Catalog`, `Archive` |
| `<pluss/scribble/scribble.h>` | the scribble model: `Slot`, `Registry`, `Axel` |
| `<pluss/bundle/bundle.h>` | the bundle model: `Nexus`, `Locus` |
| `<pluss/loader/loader.h>` | loading and saving files |

### PlussPl

`<pluss/pl.h>` is these four.

| Header | What it gives you |
|---|---|
| `<pluss/pl/studio.h>` | `Studio`, `Pane`, `Trace`: building a figure |
| `<pluss/pl/style.h>` | themes, palettes and lighting |
| `<pluss/pl/views.h>` | the views a plot window offers: smoothing, transforms, log scales, fit lines |
| `<pluss/pl/flash.h>` | the `flash` plots: a finished chart in one call |

### PlussMa

| Header | What it gives you |
|---|---|
| `<pluss/ma/math.h>` | algebra, analysis, calculus, economics, finance, numerical methods, probability |
| `<pluss/ma/stats.h>` | distributions, regression, time series, inference, Bayesian methods, estimation |
| `<pluss/ma/eng.h>` | engineering: circuits, communications, control, fluids, signals, power, robotics, structures, thermal |
| `<pluss/ma/natsci.h>` | biology, chemistry, physics |
| `<pluss/ma/actuarial.h>` | actuarial mathematics |
| `<pluss/ma/finance.h>`, `<pluss/ma/linearalg.h>` | finance and linear algebra, on their own |

### PlussAI

| Header | What it gives you |
|---|---|
| `<pluss/nn.h>` | neural modules: layers, losses, optimisers |
| `<pluss/ai/ml.h>` | classical machine learning and the lymph workflows |
| `<pluss/ai/procedure.h>` | running and saving work across the frameworks |
| `<pluss/ai/lymph.h>`, `<pluss/ai/nlp.h>` | the lymph workflows alone, and text models |

### PlussTils

`<pluss/tils.h>` is all of these.

| Header | What it gives you |
|---|---|
| `<pluss/tils/doc.h>` | reading the documentation from a program |
| `<pluss/tils/settings.h>` | settings from defaults, a file, the environment and the command line |
| `<pluss/tils/cache.h>` | keeping results between runs |
| `<pluss/tils/info.h>` | version, system and compute platforms |
| `<pluss/tils/env.h>` | reading environment variables |
| `<pluss/tils/suppose.h>`, `<pluss/tils/let.h>`, `<pluss/tils/flow.h>` | checked suppositions, immutable bindings and value flow |
| `<pluss/tils/print_options.h>`, `<pluss/tils/kit/output.h>` | printing and the output engine |
| `<pluss/tils/kit/rng_seed.h>`, `<pluss/tils/kit/pathing.h>` | seeds and paths; the other helpers are in `<pluss/tils/kit.h>` |
| `<pluss/tils/view.h>` | the built-in window behind `.show()` |
| `<pluss/bi/bi.h>` | dashboards (PlussBI) |

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run, or from the compiler's.

## Example 1: One umbrella and one opt-in piece

The program uses a `Series` and an example table, which come with `<pluss/pluss.h>`, and `Settings`, which comes from its own PlussTils header.

```cpp
// <pluss/pluss.h> gives Qit, Mic and Pl. A PlussTils piece is opt-in: include it yourself.
#include <pluss/pluss.h>
#include <pluss/tils/settings.h>

#include <iostream>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0});          // PlussQit, from pluss.h
  auto sales = pluss::table::example("sales");            // PlussMic, from pluss.h
  pluss::tils::Settings settings;                        // PlussTils, from its own header
  settings.set("rows", 3);
  std::cout << x.sum().item<double>() << " " << sales.head(settings.get<int>("rows")).ncases() << "\n";
}
```

Output

```text
6 3
```

## Pitfalls

Leaving out the opt-in include is the usual mistake. The same program without `<pluss/tils/settings.h>`:

```cpp
// The same program without the PlussTils include.
#include <pluss/pluss.h>

int main() {
  pluss::tils::Settings settings;
}
```

The compiler says

```text
ex_include_2.cpp:5:16: error: no type named 'Settings' in namespace 'pluss::tils'
```

The type is not found because `<pluss/pluss.h>` does not include PlussTils. Add the header of the piece you use, or `<pluss/tils.h>` for all of PlussTils. If you ever lose track of which header a type needs, go back to `<pluss/pluss.h>` plus the umbrella of the space you are using.

## Short aliases

Some names are long because they are precise: an option value such as `pluss::pl::flash::FlashStyle::Darkgrid`. PlussII keeps a small, approved set of short aliases for these option values and for compact option access. They follow the rules below, and every alias is registered in the repository's alias registry before it is released.

- Aliases are for option values and compact option access, never for functions or methods: there is one canonical name for each behaviour.
- An alias namespace has a lower-case name of at most four letters.
- Aliases live inside the namespace that owns them, not at the top level, and are never exposed with `using namespace` in a header.

| Alias | Stands for | Values |
|---|---|---|
| `pluss::pl::ks` | `KeyShape` | `Auto`, `Horizontal`, `Vertical` |
| `pluss::pl::flash::fc` | `FlashContext` | `Inherit`, `Paper`, `Notebook`, `Talk`, `Poster` |
| `pluss::pl::flash::fs` | `FlashStyle` | `Darkgrid`, `Lightgrid`, `Sandgrid`, `Dark`, `Light`, `Sand` |
| `pluss::pl::flash::fp` | `FlashPalette` | `Deep`, `Muted`, `Pastel`, `Bright`, `Dark`, `Colorblind` |
| `dst` (in PlussMa) | the distribution kind | `Normal`, `Lognormal`, `Uniform`, and the rest of the distributions |

The PlussPl aliases come with `<pluss/pl.h>`. The PlussMa alias is opt-in: `<pluss/ma/aliases.h>` exposes `ma::dst::Normal`, and `<pluss/aliases.h>` is an opt-in umbrella for the libraries' alias headers (today, PlussMa's). Neither is part of `<pluss/pluss.h>`.

For aliases of your own, keep them in one header in your project and alias namespaces, not functions:

```cpp
#pragma once

#include <pluss/aliases.h>  // optional: PlussII's own short aliases

namespace pm = ::pluss::ma;
namespace pp = ::pluss::pl;
namespace dist = ::pluss::subsp::stats::distr::dst;
```
