import rclpy
#Provides functions to create a
from rclpy.node import Node
from std_msgs.msg import String

class ListenerNode(Node):
    def __init__(self):
        super().__init__("listener_node")

        self.declare_parameter("topic", value="listener_topic")
        topic_name = self.get_parameter("topic").get_parameter_value().string_value

        #subscribed to the same topic which listens to the callback
        self.subscription = self.create_subscription(
            String, 
            topic_name, 
            self.listener_callback, 
            10
        )
    def listener_callback(self, msg):
        self.get_logger().info(f"Received {msg.data}")


def main(args=None):
    rclpy.init(args=args)

    #create node
    listenerNode = ListenerNode()
    #run node
    rclpy.spin(listenerNode)

    #destroy node
    ListenerNode.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()