"""PPO training entry point for humanoid locomotion.

This is intentionally a thin placeholder. Reuse the PPO, GAE, actor, and critic
pieces from the earlier lessons, then plug in one simulator wrapper from envs/.
"""


def main():
    # Load configs/humanoid_locomotion.yaml.
    # Create the source simulator environment.
    # Create actor and critic networks.
    # Collect fixed-length rollouts.
    # Compute GAE advantages and returns.
    # Run PPO clipped policy/value updates.
    # Save checkpoints to checkpoints/.
    # Export inference artifacts to exports/.
    raise NotImplementedError("Fill this after the environment wrapper is ready.")


if __name__ == "__main__":
    main()

