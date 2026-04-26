#server side file
import rclpy
from rclpy.node import Node
from custom_interfaces.srv import AddTwoInts

#node class
class AdditionService(Node):
    def __init__(self):
        #inherit Node constructor
        super().__init__("add_init_service")
        #service allows node to offer a service
        self.service = self.create_service(
            AddTwoInts,
            "add_two_ints",
            self.add_two_ints_callback
        )

    #takes req and res then adds then and returns response
    def add_two_ints_callback(self, request, response):
        response.sum = request.a+request.b
        self.get_logger().info(f"Incoming request\na:{request.a} b:{request.b}")
        return response

#main
def main(args=None):
    rclpy.init(args=args)

    #create node
    addition_service = AdditionService()
    #use/spin node
    rclpy.spin(addition_service)
    #destroy node
    addition_service.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
