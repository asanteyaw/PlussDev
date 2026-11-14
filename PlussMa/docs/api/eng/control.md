<!-- generated from control.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Control systems

The `pluss::subsp::eng::control` family turns validated LTI models into feedback laws, estimators, stability evidence, and constrained control plans. Its functions preserve Series dtype, platform, batching, and reverse sensitivity while delegating matrix equations, state estimation, and quadratic programming to their lower numerical owners.

## Branches

Use [Optimal feedback](control/optimal.md) for Riccati equations and LQR synthesis. Use [Estimation and smoothing](control/estimation.md) for steady-state estimator design, Kalman filtering, and backward smoothing. Use [Loop analysis](control/loop.md) for sensitivity functions and sampled stability margins. Use [Controller construction](control/controllers.md) for pole placement and PID controllers. Use [Model-predictive control](control/mpc.md) when finite-horizon objectives and hard bounds must be solved together.

```{toctree}
:maxdepth: 3
:hidden:

control/optimal
control/estimation
control/loop
control/controllers
control/mpc
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

control/optimal
control/estimation
control/loop
control/controllers
control/mpc
```
<!-- apiref:toctree:end -->
