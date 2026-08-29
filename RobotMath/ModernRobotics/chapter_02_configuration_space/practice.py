"""Small Chapter 2 exercises; no third-party packages required."""


def mechanism_dof(body_motion_dof: int, bodies: int, joint_dofs: list[int]) -> int:
    """Return Grübler's generic mechanism mobility count.

    ``bodies`` includes the fixed ground body. The formula assumes that all
    joint constraints are independent.
    """
    joints = len(joint_dofs)
    return body_motion_dof * (bodies - 1 - joints) + sum(joint_dofs)


def run_examples() -> None:
    examples = {
        "free planar rigid body": 3,
        "free spatial rigid body": 6,
        "planar four-bar": mechanism_dof(3, bodies=4, joint_dofs=[1, 1, 1, 1]),
        "spatial six-revolute serial arm": mechanism_dof(
            6, bodies=7, joint_dofs=[1, 1, 1, 1, 1, 1]
        ),
    }

    expected = {
        "free planar rigid body": 3,
        "free spatial rigid body": 6,
        "planar four-bar": 1,
        "spatial six-revolute serial arm": 6,
    }

    for name, dof in examples.items():
        assert dof == expected[name], f"Unexpected DOF for {name}: {dof}"
        print(f"{name}: {dof} DOF")


if __name__ == "__main__":
    run_examples()
