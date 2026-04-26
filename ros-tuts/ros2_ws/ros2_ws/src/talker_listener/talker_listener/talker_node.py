import rclpy
#Provides functions to create a ROS2 node
import rclpy.destroyable
from rclpy.node import Node

from std_msgs.msg import String

#source /opt/ros/jazzy/setup.bash

#Defines a class that inherits from Node, so now it has all the methods and properties (publishing, subscribing, logging)
class TalkerNode(Node):
    #Constructor
    def __init__(self):
        #calls the constructor
        super().__init__("talker_node")

        #Since we are passing in the topic as a parameter, we have to pass it in
        self.declare_parameter("topic", value="talker_topic")
        #Store it in a variable and change it to a string value
        topic_name = self.get_parameter("topic").get_parameter_value().string_value

        #creates a publisher and pushlished to the topic name topic with a queue of 10
        self.publisher_ = self.create_publisher(String, topic_name, 10)
        timer_period = 0.5
        #calls a timer which will call the function timer_callback every 0.5 seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.count = 0

    #Puts msg
    def timer_callback(self):
        msg = String()
        msg.data=f"Hello World {self.count}"
        self.publisher_.publish(msg)
        self.count+=1
        self.get_logger().info(f"Publishing {msg.data}")


def main(args=None):
    rclpy.init(args=args)

    #create node
    talkerNode = TalkerNode()
    #use node
    rclpy.spin(talkerNode)

    #destroy node
    talkerNode.destroy_node()
    rclpy.shutdown()


if __name__=='__main__':
    main()