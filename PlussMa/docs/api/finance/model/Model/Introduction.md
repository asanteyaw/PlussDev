<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Model` Introduction

A `Model` wraps a `std::variant` of the six parameter structs. It is a value: copy it, replace it, compare candidates. It exposes the parameters as a flat vector in the order fixed by [`Kind`](../Kind.md) (so an optimiser can move them), reports and checks its domain, and gives typed access to the held struct.

Conversions are implicit: a `Field` accepts a `model::Heston` where a `Model` is expected, and a `Model` can be assigned from any of the six structs.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="as.html">
    <span class="label">Next</span>
    <span class="title">as</span>
  </a>
</div>
<!-- apiref:nav:end -->
