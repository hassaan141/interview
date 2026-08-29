# Chapter 2 — Configuration Space

## Vocabulary

- **Configuration:** the minimum information needed to specify every point on
  a robot.
- **Degrees of freedom (DOF):** the number of independent real values needed
  to represent a configuration.
- **Configuration space (C-space):** the set of all possible configurations.
- **Constraints:** rules that remove otherwise possible motion.
- **Task space:** coordinates chosen for the task, often the end-effector pose.
- **Workspace:** the set of task-space positions or poses the robot can reach.

## Core formulas

For a mechanism with rigid bodies and joints, Grübler's formula is

`dof = m(N - 1 - J) + sum(f_i)`

where:

- `m` is the DOF of a free rigid body (`3` planar, `6` spatial),
- `N` is the number of bodies including ground,
- `J` is the number of joints,
- `f_i` is the freedom allowed by joint `i`.

This count assumes independent constraints; special geometry can make the
formula misleading.

## Lesson notes

### 2.1 Degrees of Freedom of a Rigid Body

- Planar rigid body:
- Spatial rigid body:
- Example I can explain:

### 2.2 Degrees of Freedom of a Robot

- Bodies, joints, and constraints in my example robot:
- Grübler count:
- Physical sanity check:

### 2.3 Configuration Space

- Coordinates are not the space itself because:
- Example of a C-space that wraps around:
- Implicit versus explicit representation:

### 2.4 Configuration and Velocity Constraints

- Holonomic constraint example:
- Velocity constraint example:
- How a constraint changes available motion:

### 2.5 Task Space and Workspace

- Robot configuration:
- Task coordinates:
- Reachable workspace:
- Why two configurations might produce the same task-space point:

## Retrieval questions

Answer these without notes:

1. Why does a rigid body have 3 DOF in a plane and 6 DOF in space?
2. What is the difference between a configuration and its coordinates?
3. Apply Grübler's formula to a planar four-bar linkage.
4. Give an example of a C-space that is a circle rather than a line.
5. What is the difference between configuration space and task space?
6. What is the difference between task space and workspace?
7. Why can constraint counting fail for a mechanism with special geometry?

## My summary

Write a five-sentence explanation of configuration space here after finishing
the chapter.
