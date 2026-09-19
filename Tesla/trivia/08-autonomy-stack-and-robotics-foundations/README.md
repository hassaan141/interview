# 08 — Autonomy Stack and Robotics Foundations

> "building robust code foundations for the **autonomy teams** to write their applications
> on top of"

You are not interviewing for a perception or planning role — you are interviewing to build
the substrate those teams stand on. So the goal here is **enough fluency in their domain to
design good interfaces for them**, plus real depth on the parts that are genuinely yours:
determinism, time, coordinate frames, data transport, and replay.

Your WATonomous robot-arm and ROS2 background is directly relevant; use it.

---

## Questions

### The stack
1. Name the stages of a typical autonomy pipeline, sensor to actuator.
2. Where does latency accumulate, and which part is safety-relevant?
3. What is the difference between perception, prediction, planning, and control?
4. What does a foundations/platform team own that the application teams do not?

### Time and synchronization
5. Why is `system_clock` the wrong clock for almost everything here?
6. What is sensor time vs. arrival time vs. processing time? Which do you timestamp with?
7. Two sensors at different rates must be fused. How do you associate their data?
8. What is PTP/gPTP and why does a vehicle need it?
9. What is time synchronization drift and what does it do to a fusion algorithm?

### Coordinate frames and geometry
10. What is a coordinate frame tree? What does a transform between two frames represent?
11. Why do rotations not commute, and what does that mean for your API?
12. Quaternion vs. rotation matrix vs. Euler angles — trade-offs?
13. What is gimbal lock and when does it actually bite you?
14. Why must you renormalize a quaternion, and what happens if you do not?
15. What does it mean for a transform to be "stale", and how should an API expose that?

### Data transport and middleware
16. What does a middleware like ROS2/DDS actually give you, and what does it cost?
17. Publisher/subscriber vs. request/response vs. shared memory — when each?
18. What is zero-copy transport and what does it require of your message types?
19. How do you handle a slow subscriber? Name three policies and their consequences.
20. What is back-pressure and why can a real-time producer not accept it?
21. How would you design the message type for a camera frame?

### Determinism, testing, evaluation
22. Why does bit-exact replay matter, and what breaks it?
23. Name five sources of non-determinism in a multithreaded pipeline.
24. How do you make a floating-point reduction deterministic?
25. What is hardware-in-the-loop and what does it catch that simulation does not?
26. What metrics would you report for a perception stage? For the whole stack?
27. How do you test a component that depends on time?

### Safety and robustness
28. What is a watchdog, and what should it do on expiry?
29. What is graceful degradation in this context? Give an example.
30. A sensor starts producing NaNs. Where should that be caught, and what should happen?

---
---

# Answers

**1.** Sensing (camera/radar/lidar/IMU/wheel odometry) → **ingest and timestamping** →
preprocessing (undistort, rectify, filter) → **perception** (detection, segmentation,
tracking) → fusion and **state estimation** (localization, ego-motion) → **prediction** of
other agents → **planning** (route, behaviour, trajectory) → **control** (trajectory
tracking, actuator commands) → actuation. Plus the cross-cutting layers: logging,
monitoring, watchdogs, and the calibration/config that everything reads.

**2.** Latency accumulates at every queue, every copy, every thread handoff, and every
synchronization point — not just in the compute. The safety-relevant number is **end-to-end
sensor-exposure-time → actuator-command latency at the tail (p99.9)**, because that is how
stale the world model is when the vehicle acts on it. A pipeline can have excellent
throughput and unsafe latency; improving throughput by adding stages usually makes latency
worse.

**3.** **Perception**: what is there now, from sensor data (detection, segmentation,
tracking). **Prediction**: what those agents will do next (trajectory forecasting).
**Planning**: what *we* should do — route, manoeuvre, and a trajectory that is safe and
comfortable. **Control**: how to make the actuators follow that trajectory given the
vehicle's dynamics. Each consumes the previous one's output plus the estimated ego state.

**4.** The substrate: data transport and message types, the threading and scheduling model,
memory and allocation strategy, the time/clock abstraction, coordinate-frame machinery,
logging and tracing, the build system and toolchain, testing and replay infrastructure, and
the performance tooling. The application teams own the algorithms; you own **everything that
makes their algorithms run deterministically, fast, and observably** — and the API quality
that stops them from making lifetime and concurrency mistakes.

**5.** `system_clock` is wall-clock time: it can jump backwards or forwards from NTP,
daylight saving, or a manual set, so a duration computed from it can be negative or wrong.
Use `steady_clock` (monotonic) for durations and timeouts, and a **hardware/sensor clock**
for correlating data across devices. `system_clock` is only for human-readable timestamps —
and in a logged system, for mapping back to when the drive happened.

**6.** **Sensor time** is when the measurement was physically taken (shutter exposure, lidar
sweep instant) — this is what you timestamp with, because it is what the data describes.
**Arrival time** is when the bytes reached your process (includes bus and driver latency).
**Processing time** is when you handled it. Using arrival time for fusion introduces an
error equal to the variable transport delay, which is exactly the jitter you cannot correct
for later. Sensors that can hardware-timestamp should; everything else needs a calibrated
offset.

**7.** You cannot assume samples line up. Options: **interpolate** the higher-rate signal to
the lower-rate one's timestamps (fine for smooth quantities like IMU-integrated pose,
wrong for discrete detections); **buffer and match** within a time tolerance (a ring buffer
per sensor plus a nearest-timestamp search, rejecting anything outside the tolerance); or
**motion-compensate** — transform the older measurement forward using the estimated
ego-motion, which is what a real fusion stack does. The foundations job is to provide the
time-indexed buffer and the query API so every team does it the same way.

**8.** IEEE 1588 Precision Time Protocol (gPTP is the automotive/AVB profile over Ethernet):
it synchronizes clocks across ECUs and sensors to sub-microsecond accuracy using hardware
timestamping in the MAC. A vehicle needs it because fusion, and any cross-sensor
association, is only as good as the common time base — a 10 ms clock offset at 30 m/s is a
30 cm position error in the fused result.

**9.** Drift is the slow divergence of two clocks (from crystal frequency error and
temperature). In fusion it manifests as a **systematic, growing spatial offset** between
sensors, which a filter will happily absorb as a bias in the state or a phantom velocity —
so it does not look like a clock bug, it looks like a perception bug. That is why you
monitor the sync offset as a first-class metric and alarm on it rather than debugging the
filter.

**10.** A tree (or forest) of named frames — `map` → `odom` → `base_link` → `camera_front`
— where each edge is a rigid transform. A transform `T_a_b` represents the pose of frame `b`
expressed in frame `a`, and equivalently the operation that converts a point expressed in
`b` into `a`. Keeping the naming convention consistent (`T_target_source`) is the single
most effective way to prevent transform bugs, because then composition just reads:
`T_a_c = T_a_b * T_b_c`.

**11.** Rotation is composition of orthogonal matrices, and matrix multiplication is not
commutative — physically, rotating 90° about x then 90° about y lands somewhere different
from the reverse. For your API that means **order must be explicit and unambiguous**: name
the convention (intrinsic vs. extrinsic, the axis order), prefer a type that composes
correctly (a quaternion or matrix type with `operator*`) over passing three loose angles,
and never let a caller supply "roll, pitch, yaw" without the convention being in the type
or the name.

**12.** **Rotation matrix** (9 numbers): composes and applies directly, no ambiguity, but
redundant and must be re-orthonormalized after accumulation. **Quaternion** (4 numbers):
compact, composes cheaply, interpolates correctly (SLERP), no gimbal lock — but has a
double cover (`q` and `−q` are the same rotation) which trips up naive comparison and
averaging. **Euler angles** (3 numbers): human-readable and minimal, but suffer **gimbal
lock**, have ~24 conventions, and interpolate badly. Use quaternions internally, matrices
for applying to many points, and Euler only at a human interface.

**13.** The loss of one degree of freedom when two rotation axes align — for ZYX Euler
angles, at pitch = ±90° the yaw and roll axes coincide, so the representation becomes
singular and the derivative blows up. It bites when something can actually reach that
attitude (a drone, a robot wrist, a gimballed sensor) or when a filter's state is
parameterized in Euler angles and passes near the singularity. For a ground vehicle pitch
stays small, so it usually does not — which is exactly why people are surprised when the
robot arm hits it.

**14.** Because repeated multiplication accumulates floating-point error, so the quaternion
slowly stops being unit-norm and no longer represents a pure rotation — it starts scaling
vectors it transforms, and the error compounds. Renormalize after composition (or every N
steps); the same applies to re-orthonormalizing a rotation matrix (Gram-Schmidt or an SVD
projection). This is a concrete instance of the numerical-hygiene point in course section 01.

**15.** Stale means the transform's timestamp is older than the data you want to apply it to
by more than the tolerance — the vehicle has moved since. An API should therefore make time
a **required argument**, not an optional one: `lookup(target, source, at_time)` returning
`std::expected<Transform, LookupError>` with a distinct `Extrapolation`/`TooOld` error,
rather than silently returning the latest. Making staleness impossible to ignore in the type
is exactly the kind of interface a foundations team should be providing.

**16.** It gives you: a **transport abstraction** (in-process, shared memory, or network,
chosen by configuration), **serialization** of typed messages, **discovery** so nodes find
each other, quality-of-service policies (reliability, durability, history depth), and
introspection/recording tooling for free. It costs: serialization and copies (unless
zero-copy), a discovery protocol whose behaviour at scale is its own subject, extra threads
and queues you do not control, latency and jitter that are hard to bound, and a large
dependency. For a hard-real-time inner loop you usually bypass it entirely and use shared
memory directly; the middleware carries everything else.

**17.** **Pub/sub** for streaming sensor data and state to many consumers whose identity you
do not want to know — decoupled, but no delivery guarantee to any particular consumer.
**Request/response** for a query with an answer (read a parameter, trigger a calibration) —
but never on a real-time path, because it couples your latency to someone else's.
**Shared memory** for large, high-rate data where copying is the cost that matters — a
camera frame at 30 fps is 100+ MB/s per consumer if you copy. In practice: shared memory for
the data plane, pub/sub or request/response for the control plane.

**18.** Transporting data without copying it out of the producer's buffer — the consumer
reads the same physical pages. It requires: a message type that is **trivially copyable and
standard layout** with fixed-width types and **no pointers or references** (use offsets or
indices, since addresses differ across processes); a fixed or bounded size so it can live in
a preallocated pool; the producer to **loan** a buffer from the transport rather than
allocating its own; and a lifetime protocol so the producer knows when every consumer has
finished with the slot. In ROS2 terms, that is loaned messages plus a POD-only message
definition.

**19.** (a) **Block the producer** — preserves every sample, but couples the fast producer
to the slow consumer and can stall a real-time thread: unacceptable on a sensor path.
(b) **Drop the newest** — the consumer keeps working on older data; simple, but the world
model goes stale, which is the worst failure for autonomy. (c) **Drop the oldest / overwrite
(ring buffer)** — the consumer always gets the freshest data and lateness is bounded;
usually the right answer for perception. Whichever you choose, **count the drops and export
them as a metric** — the real failure is dropping silently. A fourth option is to drop
*resolution* rather than frames (process a downsampled image), which degrades gracefully.

**20.** Back-pressure is a consumer signalling the producer to slow down. A real-time
producer cannot accept it because it is driven by the **physical world**: the camera
shutter fires at 30 Hz whether or not you are ready, and blocking the ingest thread means
missing the next frame and eventually overflowing a driver buffer. So back-pressure has to
be converted into a **drop policy plus a metric** at the boundary, and the system is
designed so the consumer's lateness degrades quality rather than breaking the pipeline.

**21.** A header of fixed-width POD fields — sequence number, **sensor exposure timestamp**
(not arrival), camera/frame ID, width, height, stride, pixel format enum with a fixed
underlying type, and the coordinate frame name or ID — plus the pixels referenced as a
`std::span<const std::byte>` into a pool-allocated buffer rather than owned inline.
`static_assert` the header is standard layout and trivially copyable, pin its `sizeof` and
field offsets, and version it. Ownership is a **lease** (an RAII handle into the buffer
pool) so the slot is returned on every exit path, and the consumer API takes a view so no
copy is implied. Include the intrinsics/distortion model by reference (an ID into a
calibration store), not by value, so the frame stays small.

**22.** Because **replay is how you evaluate changes**: you run a candidate build over
recorded drives and compare metrics against a baseline, and if the same input produces
different output run to run, you cannot attribute a metric change to your commit. It is also
how you debug a field incident — you re-run the exact scenario offline. It is broken by:
thread scheduling affecting the order of a reduction or a container, unseeded RNG,
wall-clock or "now()" reads, uninitialized memory, hash-map iteration order, pointer values
leaking into logic, floating-point reassociation under different vectorization, and any
dependence on CPU count.

**23.** (1) **Thread scheduling** — the order in which parallel results are combined.
(2) **Floating-point reassociation** in a parallel or vectorized reduction (non-associative,
course section 01). (3) **Unseeded or per-thread RNG.** (4) **Time**: any `now()` read that
influences a decision. (5) **Container iteration order** — `unordered_map` depends on hash
and insertion history; pointer-keyed containers depend on allocation addresses.
(6) Uninitialized reads. (7) Drop policies that depend on real-time arrival. The fixes:
fixed-shape reductions, injected clocks and RNGs, ordered containers or sorted keys, and
running the evaluation pipeline in a deterministic mode where queues are drained rather than
timed.

**24.** Fix the **order of combination** independently of the thread count: a
tree/pairwise reduction with a fixed chunk size, so 1 thread and 16 threads produce the same
grouping. Alternatives: accumulate in higher precision (`float` data into `double`), use
compensated summation (Kahan/Neumaier), or accumulate in fixed-point/integer where
associativity is exact. Also disable `-ffast-math` and pin `-ffp-contract` so the compiler
cannot silently change the grouping or fuse into an FMA.

**25.** Running the real software on the **real target hardware** with simulated or replayed
inputs. It catches everything that depends on the actual SoC and I/O: real latency and
jitter under real scheduling, thermal throttling and sustained-throughput limits, accelerator
behaviour and operator fallbacks, driver and DMA issues, memory bandwidth contention,
bus/CAN timing, and integration bugs between components that only appear with real timing.
A workstation simulation validates *logic*; HIL validates *timing and resources*, which is
where a foundations engineer's work actually lives.

**26.** **Per stage**: latency distribution (p50/p99/p99.9/max, not mean), throughput,
input/output queue depths, drop counts, allocation counts, CPU and accelerator utilization,
memory high-water mark. **Functional, for perception**: precision/recall per class and per
range band, tracking ID switches and track fragmentation, false-positive rate in critical
regions. **Whole stack**: end-to-end sensor→actuation latency at the tail, frame drop rate,
interventions or safety-relevant events per distance in replay/sim, determinism (is the
replay bit-identical?), and resource headroom. Report the **worst scenarios**, not just the
aggregate — a mean hides the safety-relevant tail.

**27.** **Inject the clock.** Make the component take a clock as a template parameter or an
interface, use a `FakeClock` in tests, and advance it explicitly — no `sleep`, no
wall-clock dependence, fully deterministic and fast. See
`../../cpp-course/13-debugging-testing-and-sanitizers/examples.cpp` (`RateLimiter` with
`FakeClock`) and `../../leetcode/17-design-and-systems-questions/03-rate-limiter.cpp`. The
same pattern applies to randomness (inject the generator) and to "now" in log timestamps.

**28.** A watchdog is an independent timer that a monitored component must periodically
"kick"; if the kick does not arrive within the deadline, the watchdog fires. On expiry it
should move the system to a **defined safe state**, not just log: for a vehicle that means
handing control to a redundant path or executing a minimal-risk manoeuvre, and recording
enough state to diagnose it afterwards. Key design points: the watchdog must be independent
of the thing it watches (ideally hardware), a kick must mean "I am making progress" not "I am
still scheduled", and the timeout must be derived from the deadline, not guessed.

**29.** Continuing to operate with reduced capability instead of failing outright. Examples:
one camera fails, so the system narrows the operating envelope and requests driver takeover
rather than stopping dead; the perception stage misses its deadline, so the planner uses the
previous world model with an increased uncertainty bound and a more conservative trajectory;
an accelerator fault forces a CPU fallback at lower frame rate, so resolution drops rather
than the pipeline stalling. The foundations contribution is making degradation *expressible*:
every data product carries a validity/staleness/confidence field, so consumers can reason
about missing inputs rather than crashing or silently using garbage.

**30.** Catch it **at the boundary where the data enters your control** — in the driver or
ingest layer, validating before the value ever reaches an algorithm. What should happen:
reject the sample (do not propagate NaN, because it silently poisons every downstream
computation — `NaN` compares false against every threshold, so range checks *pass*),
increment a per-sensor error counter and emit a health/diagnostic event, mark the sensor's
output as invalid so fusion can down-weight or exclude it, and if the rate exceeds a
threshold, declare the sensor failed and degrade gracefully (29). What should **not** happen:
an `assert` that is compiled out in release, a silent clamp that hides the fault, or letting
it reach the planner. And the detection must use `std::isnan`, not `x != x`, because
`-ffast-math` deletes the latter (course section 01).
