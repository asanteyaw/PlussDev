<!-- generated from control.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Control

Control is the design of a feedback law for a system whose model you know. The chapters build on the [linear systems](lti.md) models:

1. **How do I choose a feedback gain that is optimal for a cost?** The linear-quadratic regulator, its Riccati equation, and a simulation of the loop it creates.
2. **How do I estimate the states from noisy outputs?** The steady-state estimator, the Kalman filter and the smoother.
3. **How do I design by specification, and how robust is the result?** Pole placement, PID controllers, and the gain, phase and stability margins of a loop.
4. **How do I control a system with limits on its inputs and states?** Model-predictive control: a finite-horizon optimisation solved at every step.

Every function accepts batches of plants, works in single or double precision, and returns the matrices a design needs: the cost matrix, the gain and the closed-loop dynamics.

Chapters, in reading order:

- [Control 1: Optimal Feedback](control/control_01_optimal_feedback.md)
- [Control 2: Estimation](control/control_02_estimation.md)
- [Control 3: Classical Design](control/control_03_classical_design.md)
- [Control 4: Model-Predictive Control](control/control_04_model_predictive_control.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

control/control_01_optimal_feedback
control/control_02_estimation
control/control_03_classical_design
control/control_04_model_predictive_control
```
<!-- apiref:toctree:end -->
