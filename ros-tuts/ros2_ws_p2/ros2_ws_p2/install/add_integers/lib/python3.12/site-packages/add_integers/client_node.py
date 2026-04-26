import sys  # For taking values from the command line
import rclpy
from rclpy.node import Node
from custom_interfaces.srv import AddTwoInts

# Need an async class
class AdditionClientAsync(Node):
	def __init__(self):
		super().__init__("addition_client_async")
		# Creating a client with the same definition and name as the service node
		self.client = self.create_client(AddTwoInts, "add_two_ints")
		# Check if the service is active; if not, wait
		while not self.client.wait_for_service(timeout_sec=1.0):
			self.get_logger().info("Service is not available, waiting again...")

	# When sending a request, we want to pack the numbers a and b 
	def send_request(self):
		request = AddTwoInts.Request()
		request.a = int(sys.argv[1])  # First command-line argument
		request.b = int(sys.argv[2])  # Second command-line argument
		self.future = self.client.call_async(request)

def main(args=None):
	rclpy.init(args=args)

	# Create the client node
	addition_client = AdditionClientAsync()
	addition_client.send_request()

	# Spin the node until the future is complete when it is ok
	while rclpy.ok():
		rclpy.spin_once(addition_client)
		if addition_client.future.done():
			try:
				response = addition_client.future.result()
			except Exception as e:
				addition_client.get_logger().info(
					f"service call failed {e}"
				)
			else:
				addition_client.get_logger().info(
					f"result of addition is {response.sum}"
				)
			break
	# Destroy the node explicitly
	addition_client.destroy_node()
	rclpy.shutdown()

if __name__ == "__main__":
	main()
