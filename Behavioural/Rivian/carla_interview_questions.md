# CARLA Interview Questions (and what strong answers cover)

This is a practical question bank for simulation/autonomy interviews. Use it to practice “explain it like I built it”, and to prepare examples from your own work (scenario setup, sensors, log replay, ROS integration, metrics).

---

## 1) CARLA basics (the 30,000-foot view)

### Q: What is CARLA?

What a strong answer covers:

- Open-source autonomous driving simulator built on Unreal Engine.
- Purpose: safely test autonomy stacks with repeatable scenarios and rich sensor simulation.
- Provides world (maps, traffic actors, weather), sensors (RGB, depth, semantic seg, LiDAR, IMU, GNSS), and ground truth (poses, bounding boxes, labels).

### Q: Why use CARLA instead of recording real-world data?

What a strong answer covers:

- Safety and cost: long-tail/unsafe scenarios are cheap and safe in sim.
- Control: change weather, lighting, traffic density, actor behaviors.
- Ground truth: perfect labels and full state are available.
- Repeatability: deterministically replay the same scenario for regression.

### Q: What are the limitations of CARLA?

What a strong answer covers:

- Sim-to-real gap: rendering, physics simplifications, sensor artifacts.
- Domain mismatch: real sensors have quirks (latency, rolling shutter, miscalibration, time sync issues).
- Determinism can break if not careful (asynchronous stepping, multi-threading, GPU nondeterminism).

---

## 2) Architecture & core concepts

### Q: What is the CARLA “server” vs “client” model?

What a strong answer covers:

- CARLA runs as a server (Unreal simulation loop). Python API (or other clients) connect to control the world.
- Client spawns actors/sensors, sets weather, ticks the world, and pulls sensor data.

### Q: What is synchronous mode and why does it matter?

What a strong answer covers:

- **Synchronous mode**: simulation advances only when the client calls `world.tick()`.
- Benefits: determinism, consistent sensor time stamps, reproducible experiments.
- Without sync: sensors can drift and you can’t reliably align streams.

### Q: What is “fixed delta seconds” and how does it affect sensor data?

What a strong answer covers:

- Fixed timestep (e.g., 0.05s) makes physics and sensors step at a stable rate.
- Helps reproducibility and alignment for metrics; can reduce jitter.

---

## 3) Sensors, ground truth, and what each is used for

### Q: What’s the difference between RGB, depth, and semantic segmentation sensors?

What a strong answer covers:

- **RGB**: color image for perception (detection/segmentation/scene understanding).
- **Depth**: per-pixel distance to camera; useful for geometry, 3D reasoning, and validation.
- **Semantic segmentation**: per-pixel class labels (road, vehicle, pedestrian…); great for perfect masks and structured evaluation.

### Q: How does CARLA simulate LiDAR and what are common pitfalls?

What a strong answer covers:

- Raycasting-based point cloud simulation; configurable channels, points/sec, range, noise.
- Pitfalls: coordinate frames, timestamp alignment, intensity modeling differences vs real sensors.

### Q: How would you time-synchronize multiple sensors?

What a strong answer covers:

- In synchronous mode, advance the world by ticks and treat the tick as a synchronization barrier.
- Use a common simulation timestamp and per-sensor frame index.
- In ROS: ensure consistent `header.stamp` usage and define a clear TF tree.

---

## 4) Coordinate frames and transforms

### Q: What coordinate frames does CARLA use and what mistakes happen when integrating with ROS?

What a strong answer covers:

- CARLA uses Unreal’s coordinate conventions (left/right-handed details vary by integration layer).
- ROS uses REP-103 (typically: x forward, y left, z up) conventions.
- Common issues: sign flips, yaw direction mismatch, units mismatch, and wrong frame ids.

### Q: How do you validate your transforms are correct?

What a strong answer covers:

- Visualize in RViz: check vehicle heading vs velocity vector.
- Sanity-check: drive forward → x increases, turn left → yaw increases (per your convention).
- Compare against ground truth pose.

---

## 5) Scenario creation, actor control, and “long-tail” testing

### Q: What is a “scenario” in CARLA?

What a strong answer covers:

- A definition of: map, initial actor states, behavior scripts/trajectories, and environment (weather/time-of-day).
- Ideally reproducible with a seed + recorded script/log.

### Q: How would you generate long-tail scenarios?

What a strong answer covers:

- Parameterize scenarios (traffic density, occlusions, unusual merges, sensor dropout, rare behaviors).
- Use targeted sampling (importance sampling) instead of uniform random.
- Add adversarial actors or policy-driven scenario generation.

### Q: How do you control NPC vehicles?

What a strong answer covers:

- Built-in Traffic Manager for high-level behavior.
- Or manual control: waypoints / direct throttle-steer-brake commands.
- For evaluation: you often want deterministic scripted actors.

---

## 6) Log replay, determinism, and regression testing

### Q: What is “log replay” in simulation?

What a strong answer covers:

- Replaying a recorded drive (ego + actors + environment) deterministically.
- Used to reproduce bugs and compare model versions apples-to-apples.

### Q: What breaks determinism in CARLA pipelines?

What a strong answer covers:

- Async stepping or variable dt.
- Nondeterministic actor controllers.
- Multi-machine execution differences, GPU nondeterminism, threading.
- Random seeds not captured.

### Q: How would you design a regression test for an autonomy model using CARLA?

What a strong answer covers:

- Fix map + initial states + seed + sync mode.
- Define success metrics and thresholds.
- Run on each model change; store artifacts (video + metrics + traces).

---

## 7) Metrics: beyond collision

### Q: What metrics would you track beyond “no collision”?

What a strong answer covers:

- Comfort: jerk, lateral acceleration, harsh braking events.
- Rule adherence: speed limits, stop lines, red lights, lane keeping.
- Semantic correctness: yielding behavior, gap acceptance.
- Progress: time-to-goal, route completion.
- Intervention counts (if using safety driver / fallback policy).

### Q: What is open-loop vs closed-loop evaluation?

What a strong answer covers:

- Open-loop: compare predictions to recorded ground truth (no feedback into world).
- Closed-loop: policy actions affect future states (more realistic, harder).

---

## 8) Perception in simulation and sim-to-real

### Q: Why is simulated perception “easier” than real-world perception?

What a strong answer covers:

- Cleaner signals, fewer sensor defects.
- Lighting/material realism may be limited.
- Labels are perfect; in real life they’re noisy/incomplete.

### Q: How do you reduce the sim-to-real gap?

What a strong answer covers:

- Domain randomization (weather, lighting, textures).
- Sensor noise models (motion blur, latency, dropout).
- Style transfer / augmentation (sim → more photorealistic).
- Train with mixed sim + real data.

---

## 9) ROS integration and autonomy stack wiring

### Q: How does CARLA typically integrate with ROS2?

What a strong answer covers:

- A bridge publishes simulated sensor topics and TF frames.
- Autonomy stack subscribes like it would on a real vehicle.
- Key: consistent timestamps, TF tree, and message types.

### Q: How would you debug “my odometry looks wrong” in CARLA + ROS?

What a strong answer covers:

- Check frame ids and TF tree.
- Validate units (m/s vs km/h, degrees vs radians).
- Compare against CARLA ground truth odometry.
- Inspect sign conventions (steer sign, yaw direction).

---

## 10) Performance, scaling, and automation

### Q: What are common performance bottlenecks?

What a strong answer covers:

- Rendering cost (high-res cameras, many sensors).
- CPU cost (many actors, heavy traffic manager settings).
- Data logging throughput (disk, serialization, compression).

### Q: How do you scale simulations?

What a strong answer covers:

- Run headless/offscreen.
- Batch runs with orchestration (Docker/K8s, job queues).
- Shard scenarios across machines; keep configs immutable.
- Save minimal artifacts needed for evaluation (metrics + selected videos).

---

## 11) Practical “tell me about a project” prompts (use your repo work)

### Q: Tell me about a time you used CARLA in an autonomy pipeline.

Strong structure:

1. Goal (what you needed to validate).
2. Scenario (map, actors, sensors).
3. Integration (ROS topics/TF, stack components).
4. Metrics (what you measured).
5. Debug story (what broke and how you proved the fix).

### Q: How did you validate your localization/odometry output in simulation?

What a strong answer covers:

- Compare to CARLA ground truth.
- Visualize trajectory overlay.
- Check short-horizon consistency (velocity direction matches heading).
- Quantify error over time (drift rate).

---

## 12) Rapid-fire: short questions they love to ask

- What is synchronous mode?
- What’s the difference between semantic and instance segmentation?
- What do you log to reproduce a simulation run?
- How do you ensure time alignment across sensors?
- What metrics indicate uncomfortable driving?
- What’s your strategy for long-tail scenario generation?
- How do you handle nondeterminism?
- How do you integrate CARLA with ROS2?

---

## 13) “Red flag” answers to avoid

- “CARLA is just a game engine.” (Downplays simulation rigor.)
- “We only check collision.” (Metrics are too shallow.)
- “Determinism doesn’t matter.” (Regression/evaluation requires repeatability.)
- “We don’t track timestamps carefully.” (This breaks sensor fusion.)

---

## 14) Practice template (fill this in before the interview)

Write a 5–7 sentence version of your CARLA story:

- Scenario:
- Sensors:
- Outputs/Topics:
- Metrics:
- Biggest bug + fix:
- Result/impact:
