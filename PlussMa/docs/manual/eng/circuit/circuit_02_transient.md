<!-- generated from circuit_02_transient.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Circuits 2: Transient Response

A circuit with capacitors and inductors does not jump to its final state: voltages and currents move towards it over time, and a resonant circuit rings. This chapter computes that response in time with `circuit::tran`: from a switch-on or from a state you choose, under constant sources or any waveform, with two time-stepping rules whose behaviour is shown against closed forms.

- [Step response and starting state](circuit_02_transient/step_response.md)
- [Time-varying sources](circuit_02_transient/time_varying_sources.md)
- [Schemes and step size](circuit_02_transient/schemes_and_step_size.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

circuit_02_transient/step_response
circuit_02_transient/time_varying_sources
circuit_02_transient/schemes_and_step_size
```
<!-- apiref:toctree:end -->
