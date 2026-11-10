<!-- generated from field.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The pricing field

A `module::Field` bundles an instrument, a market, a model, the pricing controls and (optionally) simulated paths into one value, so that one object prices, differentiates, simulates and calibrates. Every member is plain data made of `Series`: change one and price again. The model is a [`Model`](model/Model.md) holding one of six families; the engine ([`Engine`](field/Engine.md)) is a choice, never a silent consequence, and a `Field` reports the engines a model supports when you ask for another.

| You want | Use |
|---|---|
| Prices (and Greeks) of a strip of options | [`Field::price`](field/Field/price.md) |
| To try other parameters without touching the field | `Field::price(candidate)` on the same page |
| The mismatch to quotes, a loss, an objective for a minimiser | [`residuals`](field/Field/residuals.md), [`loss`](field/Field/loss.md), [`objective`](field/Field/objective.md), [`bind_objective`](field/Field/bind_objective.md) |
| Simulated paths for path-dependent questions | [`simulate`](field/Field/simulate.md), [`simulate_paths`](field/Field/simulate_paths.md) |
| What an engine does and its settings | [`Controls`](field/Controls.md), [`Engine`](field/Engine.md) |

## The Surface

| Symbol | What it is |
|---|---|
| [`Controls`](field/Controls.md) | The engine, whether Greeks are wanted, the loss, and the settings of every engine side by side. |
| [`Engine`](field/Engine.md) | How the price of an option is computed: closed form, Fourier transforms, simulation, a tree, collocation or a PDE. |
| [`Field`](field/Field.md) | An instrument, a market, a model, pricing controls and simulated paths: the object that prices, differentiates, simulates and calibrates. |
| [`Greeks`](field/Greeks.md) | The five Greeks of a priced strip, each a `Series` with one entry per option. |
| [`Loss`](field/Loss.md) | How the residuals of a fit are reduced to one number. |
| [`Paths`](field/Paths.md) | The simulated paths of a Monte Carlo run: asset, volatility, variance and the shocks that drove them. |
| [`Result`](field/Result.md) | What pricing returns: the prices, the Greeks, and (for the FFT engine) the strike grid. |
| [`Risk`](field/Risk.md) | Whether pricing also returns the Greeks. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

field/Controls
field/Engine
field/Field
field/Greeks
field/Loss
field/Paths
field/Result
field/Risk
```
<!-- apiref:toctree:end -->
