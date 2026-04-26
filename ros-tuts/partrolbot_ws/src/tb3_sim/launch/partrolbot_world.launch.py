# Launch file to run the simulation including world and robot
# We already have a launch file from vcs tool but this to learn
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

def generate_launch_description():import

    # These are all the launch parameters, can also specify it in our run command 
    # Ex: ros2 launch my_package my_launch_file.launch.py x_pose:=1.0 y_pose:=2.0

    #Use this to launch robot state publisher later
    tb3_gazebo_launch_file_dir = os.path.join(
        get_package_share_directory('turtlebot3_gazebo'),
        'launch'
    )
    #Get package gazebo ros as that is the basis of spinning up everything
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')

    #Using smulation time will synchronize clocks and published time
    use_sim_time = LaunchConfiguration('use_sim_time', default='true')
    #starting position of the robot
    x_pose = LaunchConfiguration('x_pose', default='-2.0')
    y_pose = LaunchConfiguration('y_pose', default='-0.5')

    #Path of the world we want to use in simulation
    world = os.path.join(
        get_package_share_directory('turtlebot3_gazebo'),
        'worlds',
        'turtlebot3_world.world'
    )


    ##########################################################3
    #Below our are commands to run it

    #setting up server and client in gazebo
    gzerver_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_gazebo_ros, 'launch', 'gzserver.launch.py')
        ),
        #.items to convert the format from a dictionary to key value pairts
        launch_arguments={'world': world}.items()
    )

    gzclient_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_gazebo_ros, 'launch', 'gzclient.launch.py')
        )
    )

    #Setting up robot tree model 
    robot_State_lublisher_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tb3_gazebo_launch_file_dir, 'robot_state_publisher.launch.py')
        ),
        launch_arguments={'use_sim_time': use_sim_time}.items()
    )

    #Spawn turtlebot 3 
    spawn_turtlebot_cmd = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tb3_gazebo_launch_file_dir, 'spawn_turtlebot3.launch.py')
        ),
        launch_arguments={
            'x_pose' : x_pose,
            'y_pose' : y_pose
        }.items()
    )

    #Now we add all the commands we just made here 
    ld = LaunchDescription()

    ld.add_action(gzserver_cmd)
    ld.add_action(gzclient_cmd)
    ld.add_action(robot_state_publisher_cmd)
    ld.add_action(spawn_turtlebot_cmd)

    return ld

