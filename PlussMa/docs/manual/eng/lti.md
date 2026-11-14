<!-- generated from lti.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Linear systems

A linear time-invariant system is described by how its internal state evolves and what is observed of it. The chapters answer two questions:

1. **How do I describe a system, and what can I say about it before running anything?** State-space models, controllability and observability, gramians, poles, zeros and frequency response.
2. **How does it respond to an input?** Exact simulation of continuous and discrete models, from any starting state.

The same model feeds the control tools: a plant described here is what a controller is designed for. All the functions accept batches of systems and batches of inputs, and work in single or double precision, real or complex.

Chapters, in reading order:

- [Systems 1: Models and Analysis](lti/lti_01_models_and_analysis.md)
- [Systems 2: Simulation](lti/lti_02_simulation.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

lti/lti_01_models_and_analysis
lti/lti_02_simulation
```
<!-- apiref:toctree:end -->
