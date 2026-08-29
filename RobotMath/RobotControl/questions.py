
import math
import torch

def clamp_command(command: float, min_cmd: float, max_cmd: float) -> float:

    if command <= min_cmd:
        return min_cmd
    elif command >= max_cmd:
        return max_cmd
    else:
        return command
    
print(clamp_command(12.5, -10.0, 10.0) == 10.0)
print(clamp_command(-11.0, -10.0, 10.0) == -10.0)
print(clamp_command(3.0, -10.0, 10.0) == 3.0)

def position_error(target: float, measured: float) -> float:

    return target - measured

print(position_error(1.5, 1.0) == 0.5)
print(position_error(-1.0, 2.0) == -3.0)

def p_controller_limited(target_pos: float, measured_pos: float, kp: float, max_abs_cmd: float) -> float:

    error = target_pos - measured_pos
    
    nudge = kp * error

    if nudge > max_abs_cmd:
        target_pos += max_abs_cmd
    else:
        target_pos += nudge
    
    return target_pos

def estimate_velocity(positions: list[float], dt: float) -> list[float]:

    last_vel = None 
    vel_list = []

    for pos in positions: 

        if last_vel == None:
            last_vel = pos
            continue

        d_pos = pos - last_vel
        vel = d_pos / dt

        vel_list.append(vel)
        last_vel = pos
    
    return vel_list

print(estimate_velocity([0.0, 0.1, 0.4, 0.9], 0.1))


def moving_average(values: list[float], window_size: int) -> list[float]:
    """
    Smooth noisy sensor readings by averaging recent values.

    In robotics, sensors like encoders, IMUs, and force sensors can be noisy.
    A moving average filter makes the signal smoother by replacing each value
    with the average of the most recent `window_size` values.

    Example:
        values = [1, 2, 3, 4]
        window_size = 2

        output[0] = average([1]) = 1.0
        output[1] = average([1, 2]) = 1.5
        output[2] = average([2, 3]) = 2.5
        output[3] = average([3, 4]) = 3.5
    """

    # A window size of 0 or negative does not make sense.
    # Example: "average the last 0 values" is not a valid operation.
    if window_size <= 0:
        raise ValueError("window_size must be greater than 0")

    smoothed_values = []

    for i in range(len(values)):
        # We want the most recent `window_size` values ending at index i.
        #
        # Example with window_size = 3 and i = 5:
        # use values[3], values[4], values[5]
        #
        # max(0, ...) handles the beginning of the list, where we do not
        # have enough previous values yet.
        start_index = max(0, i - window_size + 1)
        end_index = i + 1

        window = values[start_index:end_index]

        # Average = sum of values / number of values.
        average = sum(window) / len(window)
        smoothed_values.append(average)

    return smoothed_values


print(moving_average([1, 2, 3, 4], 2) == [1.0, 1.5, 2.5, 3.5])
print(moving_average([10, 20, 30, 40, 50], 3))


class LowPassFilter:
    """
    Smooth a noisy sensor one measurement at a time.

    This is also called an exponential moving average.

    Formula:
        filtered = alpha * measurement + (1 - alpha) * previous_filtered

    alpha controls how much we trust the new measurement:
        alpha = 1.0 means no filtering; use the new measurement directly.
        alpha = 0.0 means ignore new measurements after the first value.
        alpha = 0.2 means move slowly toward the new measurement.

    Robotics intuition:
        This can smooth noisy encoder or IMU readings, but too much filtering
        adds delay. Delay in a feedback controller can cause sluggish behavior
        or even instability.
    """

    def __init__(self, alpha: float):
        # alpha must be between 0 and 1 because it is a weighting factor.
        if alpha < 0.0 or alpha > 1.0:
            raise ValueError("alpha must be between 0 and 1")

        self.alpha = alpha
        self.previous_filtered = None

    def update(self, measurement: float) -> float:
        # On the first measurement, there is no previous filtered value.
        # So we initialize the filter with the measurement itself.
        if self.previous_filtered is None:
            self.previous_filtered = measurement
            return measurement

        filtered = (
            self.alpha * measurement
            + (1.0 - self.alpha) * self.previous_filtered
        )

        # Store this value so the next update can use it.
        self.previous_filtered = filtered
        return filtered

    def reset(self):
        # Reset lets us reuse the filter for a new run or new sensor stream.
        self.previous_filtered = None


low_pass = LowPassFilter(alpha=0.5)
print(low_pass.update(10.0))  # first value returns directly: 10.0
print(low_pass.update(20.0))  # 0.5 * 20 + 0.5 * 10 = 15.0
print(low_pass.update(30.0))  # 0.5 * 30 + 0.5 * 15 = 22.5


def forward_kinematics_2d(link_lengths: list[float], joint_angles: list[float]) -> tuple[float, float]:
    """
    Forward kinematics:
        Given the robot's joint angles, compute where the end effector is.

    Example idea:
        For a 2-link planar arm:
            link_lengths = [l1, l2]
            joint_angles = [theta1, theta2]

        The function should return:
            (x, y)

    Interview meaning:
        "If I know the robot's joint positions, where is the hand/tool/lamp head?"
    """

    l1 = link_lengths[0]
    l2 = link_lengths[1]

    t1 = joint_angles[0]
    t2 = joint_angles[1]

    x = l1 * math.cos(t1) + l2 * math.cos(t1 + t2)
    y = l1 * math.sin(t1) + l2 * math.sin(t1 + t2)

    return x, y 

print(forward_kinematics_2d([3, 4], [17, 51]))


def inverse_kinematics_2d_torch(link_lengths, target_x, target_y):
    links = torch.tensor(link_lengths, dtype=torch.float32)
    target = torch.tensor([target_x, target_y], dtype=torch.float32)

    angles = torch.zeros(len(link_lengths), dtype=torch.float32, requires_grad=True)
    opt = torch.optim.Adam([angles], lr=0.01)

    for _ in range(1000):
        opt.zero_grad()

        total_angles = torch.cumsum(angles, dim=0)
        x = torch.sum(links * torch.cos(total_angles))
        y = torch.sum(links * torch.sin(total_angles))

        loss = torch.sum((torch.stack([x, y]) - target) ** 2)
        loss.backward()
        opt.step()

        if loss.item() < 1e-6:
            break

    return angles.detach().tolist()