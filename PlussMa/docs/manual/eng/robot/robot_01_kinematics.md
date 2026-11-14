<!-- generated from robot_01_kinematics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Robotics 1: Kinematics

A robot arm is a chain of rigid links joined by joints that turn or slide. Kinematics is the geometry of that chain without forces: where every link is for given joint values, how fast it moves for given joint rates, and which joint values put a link where you want it. This chapter describes a robot, places its links with forward kinematics, maps rates to velocities with the Jacobian, and solves the inverse problem by damped least squares, all vectorized over a batch of configurations.

- [Describing a robot](robot_01_kinematics/describing_a_robot.md)
- [Forward kinematics and Jacobians](robot_01_kinematics/forward_kinematics_and_jacobians.md)
- [Inverse kinematics](robot_01_kinematics/ik.md)

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

robot_01_kinematics/describing_a_robot
robot_01_kinematics/forward_kinematics_and_jacobians
robot_01_kinematics/ik
```
<!-- apiref:toctree:end -->
