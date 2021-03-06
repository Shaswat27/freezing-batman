/* 
 * File:   SteeringController.cpp
 * Author: samuel
 * 
 * Created on 3 January, 2014, 2:34 PM
 */

#include <control/SteeringController.hpp>

SteeringController::SteeringController() {
    path_ended = false;
    cte = delta_steer_angle = 0;
    cte_sum = cte_last = 0;
}

SteeringController::SteeringController(const SteeringController& orig) {
}

SteeringController::~SteeringController() {
}

float SteeringController::getSteeringControl() {
    double cmd_steer_angle = 0;

    if (path.poses.size() == 0 || path_ended) {
    } else {
        calculateParams();
        ROS_INFO("[auro666_plugin/SteeringController/getSteeringControl] state.rear_wheel_speed = %lf", state.rear_wheel_speed);
        ROS_INFO("[auro666_plugin/SteeringController/getSteeringControl] cte = %lf", cte);
        ROS_INFO("[auro666_plugin/SteeringController/getSteeringControl] delta_steer_angle = %lf", delta_steer_angle);

        if (state.rear_wheel_speed != 0) {
            double pid_term = pgain * cte + igain * cte_sum + dgain * (cte_last - cte);
            cmd_steer_angle = delta_steer_angle + atan2(pid_term, state.rear_wheel_speed);
        }
    }

    ROS_INFO("[auro666_plugin/SteeringController/getSteeringControl] cmd_steer_angle = %lf", cmd_steer_angle);
    return cmd_steer_angle;
}

unsigned int SteeringController::calculateClosestPoseId(geometry_msgs::Pose steer_point) {
    double min_distance = displacement(steer_point, path.poses.at(0).pose);
    unsigned int closest_pose_id = 0;
    for (unsigned int pose_id = 0; pose_id < path.poses.size(); pose_id++) {
        double distance = displacement(steer_point, path.poses.at(pose_id).pose);
        if (distance < min_distance) {
            min_distance = distance;
            closest_pose_id = pose_id;
        }
    }

    if (closest_pose_id + 1 == path.poses.size()) {
        path_ended = true;
    }

    return closest_pose_id;
}

void SteeringController::calculateParams() {
    geometry_msgs::Pose steer_point = obtainSteerPoint(pose);
    unsigned int closest_pose_id = calculateClosestPoseId(steer_point);

    delta_steer_angle = tf::getYaw(steer_point.orientation) - tf::getYaw(path.poses.at(closest_pose_id).pose.orientation);

    tf::Vector3 A(path.poses.at(closest_pose_id).pose.position.x,
                  path.poses.at(closest_pose_id).pose.position.y, 0);
    tf::Vector3 B(steer_point.position.x, steer_point.position.y, 0);
    double yaw = tf::getYaw(path.poses.at(closest_pose_id).pose.orientation);

    tf::Vector3 AC_cap(cos(yaw), sin(yaw), 0);
    tf::Vector3 AB = B - A;
    tf::Vector3 AC = (AB.dot(AC_cap)) * AC_cap;
    tf::Vector3 X = AB - AC; // Ulta
    tf::Vector3 Z(0, 0, 1);

    cte_last = cte;
    cte = ((X.cross(AC)).dot(Z) < 0 ? -1 : 1) * X.length();
    cte_sum += cte;
}

double SteeringController::displacement(geometry_msgs::Pose pose1, geometry_msgs::Pose pose2) {
    return sqrt((pose1.position.x - pose2.position.x) * (pose1.position.x - pose2.position.x) +
                (pose1.position.y - pose2.position.y) * (pose1.position.y - pose2.position.y));
}

geometry_msgs::Pose SteeringController::obtainSteerPoint(geometry_msgs::Pose pose) {
    geometry_msgs::Pose steer_point = pose;
    double theta = tf::getYaw(pose.orientation);
    steer_point.position.x += distance * cos(theta);
    steer_point.position.y += distance * sin(theta);

    return steer_point;
}