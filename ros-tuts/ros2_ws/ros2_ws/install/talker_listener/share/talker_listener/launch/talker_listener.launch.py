from launch import LaunchDescription
from launch_ros.actions import Node

TOPIC = "chatter"

def generate_launch_description():
    talker = Node(
        package = "talker_listener",
        executable="talkerNode",
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