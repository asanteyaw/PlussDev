<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Field` Introduction

A `Field` has five members, all plain data. `instrument` ([`Vanilla`](../../instrument/Vanilla.md)) says what is priced: type, strike(s), maturity(ies), exercise style. `market` ([`State`](../../instrument/State.md)) holds the spot, rate and dividend yield. `model` ([`Model`](../../model/Model.md)) holds one of Black-Scholes, SABR, Heston, Bates, rough Heston or rough Bergomi. `controls` ([`Controls`](../Controls.md)) chooses the engine, whether Greeks are wanted, the loss and the settings of every engine side by side. `paths` ([`Paths`](../Paths.md)) holds simulated paths once `simulate()` has been called.

**Broadcasting.** `strike`, `maturity`, `spot`, `rate` and `dividend_yield` are `Series` that must each have one element or the same number of elements `n`; a one-element member is shared by all options. The strip of `n` options is priced in one call, with a price (and Greeks) per element.

**Validation.** `price` and the other methods validate the whole field first, and report every problem together, naming the member (`model.Heston.kappa`, `instrument.strike`, `controls.engine`) and what is wrong. The engine is checked against the model and the exercise style, and the message lists what is supported. Parameters that are legal but suspicious (a Heston model that violates the Feller condition `2κθ ≥ σ²`) draw a warning, not an error.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="bind_objective.html">
    <span class="label">Next</span>
    <span class="title">bind_objective</span>
  </a>
</div>
<!-- apiref:nav:end -->
