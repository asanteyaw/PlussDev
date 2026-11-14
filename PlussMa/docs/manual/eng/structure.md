<!-- generated from structure.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Structures

A structure is a set of members joined at nodes, held by supports and pushed by loads. The analysis finds how it deforms, which forces the members carry, and how it vibrates, buckles and moves under changing loads.

1. **How does a truss or a frame deform and what does each member carry?** Pin-jointed bar trusses in two and three dimensions, and beam-column frames with axial, shear and bending action, solved statically for displacement, support reactions, member forces, stress and strain energy.
2. **How does it vibrate, buckle and respond to time-varying loads?** Natural frequencies and mode shapes, linearised buckling loads, time-stepping of transient loads with damping, and the steady response to a sinusoidal load at each frequency.
3. **What changes when displacements are large?** Corotational frames that follow large rotations, solved by load stepping, in statics and in time.

Chapters, in reading order:

- [Structures 1: Trusses](structure/structure_01_trusses.md)
- [Structures 2: Frames](structure/structure_02_frames.md)
- [Structures 3: Dynamics](structure/structure_03_dynamics.md)
- [Structures 4: Large Displacements](structure/structure_04_large_displacements.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

structure/structure_01_trusses
structure/structure_02_frames
structure/structure_03_dynamics
structure/structure_04_large_displacements
```
<!-- apiref:toctree:end -->
