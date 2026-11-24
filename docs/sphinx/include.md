# Which header to include

PlussII is six spaces, and one header gives you the starting set. Everything else is opt-in: you include it yourself, so a program compiles only what it uses. This page lists every header you are likely to include, what each one gives you, and how the short aliases work.

## The rule

`<pluss/pluss.h>` gives you **PlussQit, PlussMic and PlussPl**: the numbers, text and time types, the tables, and the plots. PlussMa, PlussAI and PlussTils are not in it. Add `<pluss/ma.h>`, `<pluss/ai.h>` or `<pluss/tils.h>` when you want them. Linking is separate: one CMake target, `Pluss::PlussII`, links every library, so the header you include decides what is compiled, not what is linked.

| Header | What it gives you |
|---|---|
| `<pluss/pluss.h>` | PlussQit, PlussMic and PlussPl together: the usual starting point |
| `<pluss/core.h>` | PlussQit alone: `Series`, `Note`, `Timeline`, `Duration`, `TimeIndex`, `Queue`, saving and loading, errors |
| `<pluss/mic.h>` | PlussMic: the table, kindle, scribble and bundle models and the loaders (it also brings in plotting when PlussPl is present) |
| `<pluss/pl.h>` | PlussPl: `Studio`, `Pane`, `Trace`, the styles and views, and the `flash` plots |
| `<pluss/ma.h>` | PlussMa: math, stats, engineering, natural sciences and actuarial |
| `<pluss/ai.h>` | PlussAI: the neural-network, machine-learning and procedure frameworks |
| `<pluss/tils.h>` | PlussTils: every PlussTils header, which includes the dashboards (PlussBI) when PlussMic is installed |

## Taking only a part

Each space has finer headers, so you can include just the piece you use. They are quicker to compile than the umbrellas, and they make plain what a file depends on.

| Space | Finer headers |
|---|---|
| PlussQit | `<pluss/io.h>` (save and load), `<pluss/array_file.h>` (array files), `<pluss/queue.h>`, `<pluss/exception.h>`, `<pluss/version.h>` |
| PlussMic | `<pluss/table/table.h>`, `<pluss/kindle/kindle.h>`, `<pluss/scribble/scribble.h>`, `<pluss/bundle/bundle.h>`, `<pluss/loader/loader.h>` |
| PlussPl | `<pluss/pl/studio.h>`, `<pluss/pl/style.h>`, `<pluss/pl/views.h>`, `<pluss/pl/flash.h>` |
| PlussMa | `<pluss/ma/math.h>`, `<pluss/ma/stats.h>`, `<pluss/ma/eng.h>`, `<pluss/ma/natsci.h>`, `<pluss/ma/actuarial.h>`, also `<pluss/ma/finance.h>` and `<pluss/ma/linearalg.h>` |
| PlussAI | `<pluss/nn.h>`, `<pluss/ai/ml.h>`, `<pluss/ai/procedure.h>`, `<pluss/ai/lymph.h>`, `<pluss/ai/nlp.h>` |
| PlussTils | `<pluss/tils/doc.h>`, `settings.h`, `cache.h`, `info.h`, `env.h`, `suppose.h`, `let.h`, `flow.h`, `print_options.h`, `view.h`, and the helpers under `<pluss/tils/kit/>` such as `output.h`, `rng_seed.h` and `pathing.h`; the dashboards are `<pluss/bi/bi.h>` |

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

The type is not found because `<pluss/pluss.h>` does not include PlussTils. Add the header named in that part's guide, or `<pluss/tils.h>` for all of PlussTils.

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
