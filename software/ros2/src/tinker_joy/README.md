### Джойстик для tinker

Подробный гайд: https://articulatedrobotics.xyz/tutorials/mobile-robot/applications/teleop
publishes a sensor_msgs/Joy message - essentially a list of which buttons and axes were pressed at that time

Основные команды (из гайда):

To check our gamepad works in Linux, we want to install some useful tools:
`sudo apt install joystick jstest-gtk evtest`

`evtest` (скорее всего джойстик будет последним устройством в списке)

To listen to a joystick with the new drivers, we can use the tools from the joy package. The first one will tell us which controllers ROS can see:
`ros2 run joy joy_enumerate_devices`

Now we can open up two terminals to run the two commands below - the first is running the node and the second is displaying the outputs.
`ros2 run joy joy_node` <-- Run in first terminal
`ros2 topic echo /joy` # <-- Run in second terminal

Echoing /joy is good for checking that the gamepad is working, but it isn't always easy to tell exactly what’s going on, especially counting which axis is which. I couldn't find any other obvious tools out there for this so I wrote a simple one myself, available here. It's not currently available through the package manager so you'll have to install it from source (but I may release it if there is interest).

Once installed, it can be run with ros2 run `joy_tester test_joy`

Полное описание всех возможных параметров, которые можно задать в конфиг файле:
https://github.com/ros-drivers/joystick_drivers/tree/ros2/joy
