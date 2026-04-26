from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'tb3_sim'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        # Register the package in ament index
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        # Install the package.xml
        ('share/' + package_name, ['package.xml']),
        # Install all launch files into the 'launch' directory
        (os.path.join('share', package_name, 'launch'),
        glob('launch/*.launch.[pxy][yma]*')),
        # Install map files into the 'maps' directory
        (os.path.join('share', package_name, 'maps'),
        glob('maps/*')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='hassan',
    maintainer_email='hassaanfarooqi2000@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'amcl_init_pose_publisher = tb3_sim.set_init_amcl_pose:main',
        ],
    },
)
