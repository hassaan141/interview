import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/hassan/ros-tuts/patrol_ws/src/tb3_sim/install/tb3_sim'
