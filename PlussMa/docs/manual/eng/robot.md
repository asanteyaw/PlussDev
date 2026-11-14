<!-- generated from robot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robotics

A robot arm is a tree of rigid links joined by joints. The questions are geometric and dynamic: where is every link for given joint values, how fast does a point move, which joint values put the hand where you want it, and which torques produce a motion.

1. **Where are the links, how fast do they move, and which joint values reach a target?** Robots described as a tree of revolute and prismatic joints, forward kinematics and Jacobians over a batch of configurations, and inverse kinematics by damped least squares for position and pose targets.

2. **Which torques produce a motion, and which motion do torques produce?** Inverse dynamics by recursive Newton-Euler, the inertia matrix and the bias forces, forward dynamics and a simulation whose energy is conserved.

3. **How is a robot held to its surroundings?** Points pinned in the world, frictionless unilateral contacts with planes, and the constraint forces that come out of the dynamics.

Chapters, in reading order:

- [Robotics 1: Kinematics](robot/robot_01_kinematics.md)
- [Robotics 2: Dynamics](robot/robot_02_dynamics.md)
- [Robotics 3: Constraints and Contacts](robot/robot_03_constraints.md)

Every chapter follows the same format: a mental model, complete programs, and output produced by those programs.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

robot/robot_01_kinematics
robot/robot_02_dynamics
robot/robot_03_constraints
```
<!-- apiref:toctree:end -->
