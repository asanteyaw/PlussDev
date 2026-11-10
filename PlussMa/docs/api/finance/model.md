<!-- generated from model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Models

The model a [`Field`](field/Field.md) prices: a [`Model`](model/Model.md) holds one of six families, each a plain struct of `Series` parameters, and exposes them as a flat vector so that an optimiser can move them. [`Kind`](model/Kind.md) names the families and fixes the parameter order. The structs ([`BlackScholes`](model/BlackScholes.md), [`Heston`](model/Heston.md), [`Bates`](model/Bates.md), [`Sabr`](model/Sabr.md), [`RoughHeston`](model/RoughHeston.md), [`RoughBergomi`](model/RoughBergomi.md)) document each parameter and its domain.

| You want | Use |
|---|---|
| A model from plain numbers | [`Model::make`](model/Model/make.md) |
| The parameters as a vector, or to set them from one | [`Model::parameters`](model/Model/parameters.md) |
| The concrete struct inside a `Model` | [`holds`](model/Model/holds.md), [`as`](model/Model/as.md), [`visit`](model/Model/visit.md) |
| Domains and the Feller warning | [`validate`](model/Model/validate.md) |

## The Surface

| Symbol | What it is |
|---|---|
| [`Bates`](model/Bates.md) | Heston with log-normal jumps in the asset: eight parameters. |
| [`BlackScholes`](model/BlackScholes.md) | Constant-volatility model: one parameter, the volatility. |
| [`Heston`](model/Heston.md) | The Heston stochastic-variance model: five parameters, plus a drift and a variance floor. |
| [`Kind`](model/Kind.md) | The six model families, in the order that fixes each family's parameter vector. |
| [`Model`](model/Model.md) | A value holding one of the six model families, with its parameters as a flat vector. |
| [`RoughBergomi`](model/RoughBergomi.md) | The rough Bergomi model: four parameters, plus a drift and a variance floor. |
| [`RoughHeston`](model/RoughHeston.md) | A Heston-type model with a rough variance kernel: six parameters. |
| [`Sabr`](model/Sabr.md) | The SABR stochastic alpha-beta-rho model: four parameters, plus drift, floor and the volatility convention. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/Bates
model/BlackScholes
model/Heston
model/Kind
model/Model
model/RoughBergomi
model/RoughHeston
model/Sabr
```
<!-- apiref:toctree:end -->
