<!-- generated from fluid.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fluid networks

A water distribution system is a network of pipes joining reservoirs, tanks and the junctions where people draw water, with pumps to lift the water and valves to take the head out of it. The analysis finds the pressures and flows the network settles into, how they change through a day of demand and operation, and what the water carries with it.

1. **What are the pressures and flows?** Describing a network, solving it with loops, choosing a head-loss law, and solving many scenarios at once with their sensitivities.
2. **What do pumps and valves do?** The pump curve and its operating point, and the four control valves with the settings a network can and cannot satisfy.
3. **How does it behave over a day?** Pressure-driven demand and leaks, tanks and demand patterns, and rules that switch pumps on a clock and on a level.
4. **What does the water carry?** The concentration of a constituent, the age of the water, and its temperature.

Chapters, in reading order:

- [Fluid 1: Pipe Networks](fluid/fluid_01_pipe_networks.md)
- [Fluid 2: Pumps and Valves](fluid/fluid_02_pumps_and_valves.md)
- [Fluid 3: Storage and Operation](fluid/fluid_03_storage_and_operation.md)
- [Fluid 4: Transport](fluid/fluid_04_transport.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

fluid/fluid_01_pipe_networks
fluid/fluid_02_pumps_and_valves
fluid/fluid_03_storage_and_operation
fluid/fluid_04_transport
```
<!-- apiref:toctree:end -->
