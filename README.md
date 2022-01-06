## 如何运行

Open first terminal; (opens gazebo and rviz while spawning Aliengo, conveyor and a box)

* `roslaunch aliengo_gazebo normal_aliengo.launch`

Open third terminal; (opens state machine)

* `rosrun aliengo_state_mach state_machine.py`

Open fourth terminal; (opens state-event controller) and press s to start

* `rosrun aliengo_delivery aliengo_delivery.py`

Additionally to visualize states and events

* `rosrun smach_viewer smach_viewer.py `