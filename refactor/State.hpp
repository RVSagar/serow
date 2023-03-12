/*
 * Copyright Stylianos Piperakis, Ownage Dynamics L.P.
 * License: GNU: https://www.gnu.org/licenses/gpl-3.0.html
 */
#pragma once
#include <map>
#include <string>

#include <eigen3/Eigen/Dense>

class State
{
public:
    State() = default;
    ~State() = default;
    State(State &state);
    State(State &&state);
    State &operator=(State state);
    State &operator=(State &&state);

    // State getters
    Eigen::Isometry3d getBasePose() const;
    Eigen::Vector3d getBasePosition() const;
    Eigen::Quaterniond getBaseOrientation() const;
    Eigen::Vector3d getBaseLinearVelocity() const;
    Eigen::Vector3d getBaseAngularVelocity() const;
    Eigen::Vector3d getImuAccelBias() const;
    Eigen::Vector3d getImuGyroBias() const;
    std::optional<Eigen::Isometry3d> getFootPose(const string &frame_name) const;

    // State covariance getter
    Eigen::Matrix6d getBasePoseCov() const;
    Eigen::Matrix3d getBasePositionCov() const;
    Eigen::Matrix3d getBaseOrientationCov() const;
    Eigen::Matrix3d getBaseLinearVelocityCov() const;
    Eigen::Matrix3d getBaseAngularVelocityCov() const;
    Eigen::Matrix3d getImuAccelBiasCov() const;
    Eigen::Matrix3d getImuGyroBiasCov() const;
    std::optional<Eigen::Matrix6d> getFootPoseCovariance(const string &frame_name) const;

    // State setter
    void update(State state);

private:
    // Flag to indicate if the robot is in contact with the ground
    bool is_in_contact;
    // Base pose as an transformation from world to base
    Eigen::Isometry3d base_pose_{Eigen::Isometry3d::Identity()};
    // Base position in the world frame
    Eigen::Vector3d base_pos_{Eigen::Vector3d::Zero()};
    // Base orientation in the world frame
    Eigen::Quaterniond base_orientation_{Eigen::Quaternion::Identity()};
    // Base linear velocity in the world frame
    Eigen::Vector3d base_linear_vel_{Eigen::Vector3d::Zero()};
    // Base angular velocity in the world frame
    Eigen::Vector3d base_angular_vel_{Eigen::Vector3d::Zero()};
    // Feet state: frame_name to foot pose in the world frame
    std::optional<std::map<std::string, Eigen::Isometry3d>> foot_pose_;
    // Imu acceleration bias in the local imu frame
    Eigen::Vector3d imu_accel_bias_{Eigen::Vector3d::Zero()};
    // Imu gyro bias in the local imu frame
    Eigen::Vector3d imu_gyro_bias_{Eigen::Vector3d::Zero()};

    // Covariances
    // Base pose covariance in the world frame
    Eigen::Matrix6d base_pose_cov_{Eigen::Matrix6d::Zero()};
    // Base position covariance in the world frame
    Eigen::Matrix3d base_pos_cov_{Eigen::Matrix3d::Zero()};
    // Base orientation covariance in the world frame
    Eigen::Matrix3d base_orientation_cov_{Eigen::Matrix3d::Zero()};
    // Base linear velocity covariance in the world frame
    Eigen::Matrix3d base_linear_vel_cov_{Eigen::Matrix3d::Zero()};
    // Base angular velocity covariance in the world frame
    Eigen::Matrix3d base_angular_vel_cov_{Eigen::Matrix3d::Zero()};
    // Imu acceleration bias covariance in the local imu frame
    Eigen::Matrix3d imu_accel_bias_cov_{Eigen::Matrix3d::Zero()};
    // Imu gyro rate bias covariance in the local imu frame
    Eigen::Matrix3d imu_gyro_bias_cov_{Eigen::Matrix3d::Zero()};
    // Feet state: frame_name to foot pose covariance in the world frame
    std::optional<std::map<std::string, Eigen::Matrix6d>> foot_pose_cov_;

}