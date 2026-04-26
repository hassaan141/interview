import os
import sys
import time
import unittest
import uuid

import launch
import launch_ros
import launch_ros.actions
import launch_testing.actions

import rclpy
import std_msgs.msg

#launch feature node
def generate_test_description():
    file_path = os.path.dirname(__file__)
    #launch tallker node in test mode
    talker_node = launch_ros.actions.Node(
        executable=sys.executable,
        arguments=[os.path.join(
            file_path, "..", "talker_listener", "talker_node.py")],
        #so python output are sent straght to terminal without buffer
        additional_env={'PYTHONUNBUFFERED': '1'},
        parameters=[{
            "topic":"talker_chatter"
        }]
    )

    listener_node = launch_ros.actions.Node(
        executable=sys.executable,
        arguments=[os.path.join(
            file_path, "..", "talker_listener", "listener_node.py")],
        #so python output are sent straght to terminal without buffer
        additional_env={'PYTHONUNBUFFERED': '1'},
        parameters=[{
            "topic":"talker_chatter"
        }]
    )

    #test node
    return(
        launch.LaunchDescription([
            talker_node,
            listener_node,
            #Start right away
            launch_testing.actions.ReadyToTest(),
        ]),
        {
            'talker':talker_node,
            'listener':listener_node
        }
    )

#test node
#watch video on unit testing in python
class TestTalkerListenerLink(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        #initialize ros2 for the test node
        rclpy.init()
    
    @classmethod
    def tearDownClass(cls):
        rclpy.shutdown()
    
    def setup(self):
        self.node = rclpy.create_node('test_talker_listener_link')
    
    def tearDown(self):
        self.node.destroy_node()

    def test_talker_transmits(self, talker, proc_output):
        #store messages in this list
        msgs_rx=[]
    
        sub = self.node.create_subscription(
            std_msgs.msg.String,
            'talker_chatter',
            lambda msg: msgs_rx.append(msg),
            10
        )

        try:
            #Checking to see that within the period, the talker transmits atleast 2 messages
            end_time = time.time()+10
            while time.time()<end_time:
                rclpy.spin_once(self.node, timeout_sec=0.1)
                if len(msgs_rx)>2:
                    break

            self.assertGreater(len(msgs_rx),2)

            for msg in msgs_rx:
                #to see the output of all the processes
                proc_output.assertWaitFor(
                    expected_output=msg.data, 
                    process=talker
                )
        finally:
            self.node.destroy_subscription(sub)

    def test_listener_receives(self, listener, proc_output):
        pub = self.node.create_publisher(
            std_msgs.msg.String,
            'listener_chatter',
            10
        )

        try:
            msg = std_msgs.msg.String()
            msg.data = str(uuid.uuid4())
            for _ in range(10):
                pub.publish(msg)
                success = proc_output.waitFor(
                    expected_output=msg.data,
                    process=listener,
                    timeout = 10
                )
                if success:
                    break
                assert success, 'Waiting for output timed out'
        finally:
            self.node.destroy_publisher(pub)

