# Humanoid Robot Assets

Place robot files here:

- `urdf/`: URDF robot description, for example `humanoid.urdf`.
- `mjcf/`: MuJoCo XML robot description, for example `humanoid.xml`.
- `meshes/`: mesh files referenced by URDF or MJCF.

Keep joint names and ordering consistent across URDF, MJCF, and simulator
wrappers. Most sim2sim bugs come from mismatched joint order, units, axes, or
actuator limits.

