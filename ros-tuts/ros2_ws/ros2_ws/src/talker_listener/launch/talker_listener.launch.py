from launch import LaunchDescription
from launch_ros.actions import Node

TOPIC = "chatter"

def generate_launch_description():
    talker = Node(
        #Name of our package we created, in this case it is talker_listener
        package = "talker_listener",
        #Name in our setup.py file, what we exectute
        executable="talkerNode",
        #Name is what we will call it when we use to run the launch file
        name="talker_node",
        parameters=[{
            "topic": TOPIC
        }]
    )

    listener = Node(
        package="talker_listener",
        executable="listenerNode",
        name="listener_node",
        parameters=[{
            "topic": TOPIC
        }]
    )

    return LaunchDescription([
        talker,
        listener
    ])