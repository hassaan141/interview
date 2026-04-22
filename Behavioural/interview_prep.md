# Intw Prep: Depth Estimation Acceleration and Mock Odometry Localization

## 1) One-minute summary

I worked on two perception and localization pieces in this repository:

1. I accelerated depth estimation inference by exporting the model to ONNX and then building a TensorRT engine for faster GPU execution.
2. I created a mock odometry node for CARLA simulation using a bicycle model to estimate vehicle motion from speed and steering inputs.

The common theme was turning raw simulation or model outputs into faster, more usable runtime components for autonomous driving workflows.

---

## 2) Feature 1: Faster depth estimation with ONNX / TensorRT

### What I did

- Took the depth estimation model, DepthAnythingV2.
- Exported the PyTorch model to ONNX.
- Built a TensorRT engine from the ONNX model for faster inference.

### Files involved

- [src/perception/depth_estimation/depth_estimation/export_v2.py](../src/perception/depth_estimation/depth_estimation/export_v2.py)
- [src/perception/depth_estimation/depth_estimation/onnx_to_engine.py](../src/perception/depth_estimation/depth_estimation/onnx_to_engine.py)

### Simple explanation

PyTorch is great for training and experimentation, but deployment is often faster with ONNX and TensorRT.

- ONNX is a portable model format that lets you move a model out of PyTorch into a standardized inference graph.
- TensorRT is NVIDIA’s inference optimizer/runtime that can fuse operations, optimize kernels, and run the model much faster on GPU.

### Why this improved speed

- Removed Python/PyTorch training overhead from the inference path.
- Let TensorRT optimize the graph for the target GPU.
- Reduced latency for real-time depth estimation use cases.

### How I would describe the pipeline

1. Load the trained DepthAnythingV2 weights.
2. Export the model with a dummy input to ONNX.
3. Parse the ONNX model with TensorRT.
4. Serialize the engine and run inference from the optimized engine.

### Good interview phrasing

- “I converted the model from PyTorch to ONNX so it could be deployed in a more portable inference format.”
- “Then I used TensorRT to optimize the graph and improve runtime performance on NVIDIA GPUs.”
- “The goal was lower latency and more practical real-time inference.”

### If asked about caveats

- Exporting with a fixed dummy input can lock in the input shape unless dynamic shapes are configured.
- TensorRT performance depends on the GPU, supported layers, and precision settings.

---

## 3) Feature 2: Mock odometry node for localization in CARLA

### What I did

- Built a ROS 2 node named WheelOdometry.
- Subscribed to vehicle speed, steering, and encoder-style inputs.
- Used a bicycle model to estimate pose over time.
- Published a nav_msgs/Odometry message for downstream localization and planning.
- Initialized the pose from CARLA ground-truth odometry when available.

### Files involved

- [src/world_modeling/localization/include/localization/odom.hpp](../src/world_modeling/localization/include/localization/odom.hpp)
- [src/world_modeling/localization/src/odom.cpp](../src/world_modeling/localization/src/odom.cpp)
- [src/world_modeling/localization/config/params.yaml](../src/world_modeling/localization/config/params.yaml)
- [src/world_modeling/localization/launch/localization.launch.py](../src/world_modeling/localization/launch/localization.launch.py)

### What the bicycle model is

The bicycle model is a simplified vehicle motion model.

It approximates the car as a single front wheel and a single rear wheel. Given vehicle speed $v$, steering angle $\delta$, and wheelbase $L$:

$$
\omega = \frac{v \tan(\delta)}{L}
$$

Where:

- $\omega$ is angular velocity.
- $v$ is linear velocity.
- $\delta$ is steering angle.
- $L$ is wheelbase.

Then the node integrates pose over time:

- $\theta \leftarrow \theta + \omega \Delta t$
- $x \leftarrow x + v \cos(\theta) \Delta t$
- $y \leftarrow y + v \sin(\theta) \Delta t$

In the code, the steering sign is adjusted for CARLA’s coordinate convention.

### Why this was useful

- It provided a localization-style output even when real vehicle localization was not available.
- It let other parts of the stack test against a realistic odometry stream.
- It was especially useful for simulation and bring-up in CARLA.

### What CARLA is

CARLA is an open-source autonomous driving simulator.

It provides:

- virtual roads and cities,
- vehicles and pedestrians,
- cameras, lidar, IMU, GNSS, and other sensors,
- and ground-truth data for validation.

In this repo, CARLA was used as a controlled simulation environment to test localization and perception logic without needing a real vehicle.

### What the node subscribes to

- Left wheel speed
- Right wheel speed
- Steering angle
- CARLA ego vehicle status
- Initial CARLA odometry for pose initialization

### What it publishes

- A standard ROS 2 odometry message with position, orientation, and twist

### Good interview phrasing

- “I implemented a mock localization node that approximates vehicle motion using a bicycle model.”
- “It converts velocity and steering into pose updates, which is a standard way to simulate odometry when sensor data is limited.”
- “It was designed for CARLA so we could test downstream localization and planning logic in simulation.”

### If asked about limitations

- It is an approximation, not a full state estimator.
- Errors accumulate over time because it integrates motion rather than correcting with external measurements.
- Real-world deployment would need sensor fusion, not just a bicycle model.

---

## 4) Semantic segmentation in plain language

### Definition

Semantic segmentation means assigning a class label to every pixel in an image.

Examples:

- road
- vehicle
- pedestrian
- sidewalk
- building
- sky

### Why it matters

It gives dense scene understanding, which is useful for:

- driving stacks,
- masking objects,
- generating control signals for simulation-to-real pipelines,
- and evaluating scene structure.

### How it differs from related tasks

- Classification: one label for the whole image
- Detection: boxes around objects
- Semantic segmentation: label every pixel
- Depth estimation: predict distance for pixels, not object classes

### Good interview phrasing

- “Semantic segmentation gives pixel-level scene labels, so the model knows which pixels belong to the road, car, lane, or pedestrian.”
- “It is especially useful when you want precise masks or structured scene understanding.”

---

## 5) CARLA vs. real-world data

### Why use CARLA

- Safe testing for edge cases.
- No real-world risk.
- Easy control over weather, traffic, and sensor placement.
- Ground truth is available.

### Tradeoff

- Simulation is controlled, but it does not perfectly match the real world.
- That is why sim-to-real techniques and fast inference pipelines matter.

---

## 6) Short answers for common interview questions

### What did you do?

I accelerated depth estimation inference with ONNX and TensorRT, and I implemented a CARLA-based mock odometry node using a bicycle model for localization testing.

### Why ONNX and TensorRT?

ONNX made the model portable, and TensorRT optimized it for faster GPU inference.

### Why a bicycle model?

It is a simple and effective approximation for vehicle motion using speed, steering, and wheelbase.

### Why CARLA?

CARLA provided a safe simulator with ground truth and sensor data for development and testing.

### What is semantic segmentation?

It is per-pixel classification of the scene, such as road, vehicle, or pedestrian.

### What is the main limitation of the mock odometry node?

It accumulates drift because it is an approximation-based integration model, not a full sensor-fusion estimator.

---

## 7) Slide-ready version

### Project 1: Faster depth estimation inference

- Exported DepthAnythingV2 from PyTorch to ONNX.
- Built a TensorRT engine for optimized GPU inference.
- Goal: reduce latency and make depth estimation more suitable for real-time use.

### Project 2: Mock odometry for CARLA localization

- Implemented a ROS 2 odometry node using a bicycle model.
- Used velocity and steering inputs to estimate pose over time.
- Goal: provide realistic localization output for simulation and testing.

---

## 8) Final takeaway

If I had to summarize the work in one sentence:

I helped make the perception stack faster and the simulation stack more realistic by optimizing depth inference for deployment and by building a motion-based odometry node for CARLA-based localization testing.