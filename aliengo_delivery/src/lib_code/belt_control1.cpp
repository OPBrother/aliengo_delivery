#include "aliengo_delivery/belt_control1.hpp"
// CPP
#include <stdio.h>
#include <iostream>
#include <cmath>
// ROS
#include <ros/ros.h>
#include "sensor_msgs/PointCloud2.h"
#include "nav_msgs/Odometry.h"
#include "gazebo_msgs/ApplyBodyWrench.h"
#include "aliengo_state_mach/RoverStateMsg.h"
#include "aliengo_state_mach/RoverActionMsg.h"

// PCL LIBRARY
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/features/normal_3d.h>

using namespace std;
using namespace ros;


#define PI 3.14159265

namespace aliengo_delivery {

  BeltControl::BeltControl(ros::NodeHandle nodeHandle): n_(nodeHandle) {
    //subs elevation node's cloud output
    // pcSubscriber_=n_.subscribe("/belt/cloud", 1, &BeltControl::pcCallback, this);
    // stateSubscriber_=n_.subscribe("/smach/output", 1, &BeltControl::stateCallback, this);
    // actionPublisher_=n_.advertise<aliengo_state_mach::RoverActionMsg>("/smach/input", 1);

    // updateTimer_ = n_.createTimer(ros::Duration(1/rate), &BeltControl::updateTimerCallback, this);
    forceClient   = n_.serviceClient<gazebo_msgs::ApplyBodyWrench>("/gazebo/apply_body_wrench");
    n_.getParam(ros::this_node::getName( )+"/box_distance", box_distance);
    n_.getParam(ros::this_node::getName( )+"/apply_force", apply_force);
    n_.getParam(ros::this_node::getName( )+"/control_signal", c_signal);
    inputCloud_.reset(new pcl::PointCloud<pcl::PointXYZ>);
    ROS_INFO("Belt Control  started.");

    gazebo_msgs::ApplyBodyWrench force_;
    force_.request.body_name = "box::my_box";
    force_.request.reference_frame = "belt::my_box";
    force_.request.wrench.force.x = -10;
    force_.request.wrench.force.y = 0;
    force_.request.wrench.force.z = 0;
    ros::Time t(0);
    force_.request.start_time = t;
    ros::Duration d(-1);
    force_.request.duration = d;
    // if(apply_force)
    	forceClient.call(force_);



  }

  BeltControl::~BeltControl() {
  	applyForce(0);
    n_.shutdown();
    updateTimer_.stop();
  }
  //point cloud call back used for slope calculation
  void BeltControl::pcCallback(const sensor_msgs::PointCloud2ConstPtr &laserCloudMsg) {
    inputCloud_.reset(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::fromROSMsg(*laserCloudMsg, *inputCloud_);
    if(!checkEmpty(inputCloud_))
      pc_available=true;
    else{
      ROS_INFO("input cloud is empty");
      pc_available=false;
    }
  }
  void BeltControl::stateCallback(const aliengo_state_mach::RoverStateMsg stateMsg) {
    state.state = stateMsg.state;
    state_available=true;
  }
  
  
  //update functions 
  void BeltControl::updateTimerCallback(const ros::TimerEvent& timerEvent) {
    if(pc_available && state_available){
      if(state.state == state.IDLE_IDLE_2){
        action.ACTION = action.MOVING_BOX_STARTED;
        actionPublisher_.publish(action);
      }
     if(state.state == state.IDLE_IDLE_3){
        double error = box_distance - boxDistanceCalculation(inputCloud_);
        if(abs(error)<0.1){
          action.ACTION = action.BOX_IS_ON_THE_ROBOT;
          actionPublisher_.publish(action);
        }
      }
    
      if(state.state == state.IDLE_WORKING){
        double reference = box_distance;
        bool on_belt = false;
        if(boxDistanceCalculation(inputCloud_) <=3.6)
        	on_belt = true;
        else
        	on_belt = false;
        double control_signal;
        if(on_belt){
  	      double error = reference - boxDistanceCalculation(inputCloud_);
  	      if(abs(error)<0.1){
  	        error = 0;
  	        control_signal = 0.0;
  	        if(do_once){
  	          applyForce(control_signal);
  	          do_once = false;
  	        }
            action.ACTION = action.MOVING_BOX_ENDED;
  	      }
  	      else if(error > 0){
  	       control_signal = - c_signal;
  	       applyForce(control_signal);
  	       do_once = true;

  	      }
  	      else if(error < 0){
  	       control_signal = c_signal;
  	       applyForce(control_signal);
  	       do_once = true;
  	      }
  	      cout<<error<<" "<<control_signal<<endl;
  	    }else{
  	    	applyForce(0.0);
  	    	cout<<" box is not on the belt"<<endl;
          action.ACTION = action.BOX_FALLS;

  	    }
        actionPublisher_.publish(action);
      }
      
      state_available = false;
      pc_available=false;
    }
  }

  double BeltControl::boxDistanceCalculation(pcl::PointCloud<pcl::PointXYZ>::Ptr &cloudMsg_){ 
    int size = cloudMsg_->points.size();
    double distance = 0;
    double mean = 0;
    for(int i=0; i<size; i++)
    {
      distance = sqrt(pow(cloudMsg_->points[i].x,2) +  pow(cloudMsg_->points[i].y,2) +  pow(cloudMsg_->points[i].z,2));
      mean += distance ;
    }
    mean = mean/size;
    //cout<<"Distance is "<<mean<<endl;
		return mean;
	}
  void BeltControl::applyForce(double x_){ 
    gazebo_msgs::ApplyBodyWrench force_;
    force_.request.body_name = "box::my_box";
    force_.request.reference_frame = "belt::my_box";
    force_.request.wrench.force.x = x_;
    force_.request.wrench.force.y = 0;
    force_.request.wrench.force.z = 0;
    ros::Time t(0);
    force_.request.start_time = t;
    ros::Duration d(-1);
    force_.request.duration = d;
    if(apply_force)
    	forceClient.call(force_);
     
  }
  
  //checks if point cloud is empty
  bool BeltControl::checkEmpty(pcl::PointCloud<pcl::PointXYZ>::Ptr  pc){
    for (int i=0; i<pc->points.size(); i++){
      if(!isnan(pc->points[i].x) && !isnan(pc->points[i].y) && !isnan(pc->points[i].z))
        return false;
    }
    return true;
  }
 
}  // namespace aliengo_delivery