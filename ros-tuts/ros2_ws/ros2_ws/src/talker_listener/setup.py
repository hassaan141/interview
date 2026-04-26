from setuptools import find_packages, setup
import os
from glob import glob

#we call our package name here
package_name = 'talker_listener'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        #Our code is run from the install directory not source code
        (os.path.join('share', package_name, 'launch/'),
         glob('launch/*launch.[pxy][yma]*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='hassaan',
    maintainer_email='hassaan@todo.todo',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talkerNode = talker_listener.talker_node:main',
            'listenerNode = talker_listener.listener_node:main'
        ],
    },
)
