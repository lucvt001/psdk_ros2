/*
 * Copyright (C) 2023 Unmanned Life
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file psdk_wrapper.hpp
 *
 * @brief Header file for the psdk_wrapper class
 *
 * @author Bianca Bendris
 * Contact: bianca@unmanned.life
 *
 */

#ifndef PSDK_WRAPPER_INCLUDE_PSDK_WRAPPER_PSDK_WRAPPER_HPP_
#define PSDK_WRAPPER_INCLUDE_PSDK_WRAPPER_PSDK_WRAPPER_HPP_

#include <dji_aircraft_info.h>
#include <dji_core.h>
#include <dji_flight_controller.h>
#include <dji_logger.h>
#include <dji_platform.h>
#include <dji_typedef.h>

#include <cmath>
#include <geometry_msgs/msg/accel_stamped.hpp>
#include <geometry_msgs/msg/quaternion_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <map>
#include <memory>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <sensor_msgs/msg/magnetic_field.hpp>
#include <sensor_msgs/msg/nav_sat_fix.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/u_int16.hpp>
#include <std_msgs/msg/u_int8.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <string>

#include "dji_camera_manager.h"  //NOLINT
#include "dji_gimbal_manager.h"  //NOLINT
#include "hal_network.h"         //NOLINT
#include "hal_uart.h"            //NOLINT
#include "hal_usb_bulk.h"        //NOLINT
#include "osal.h"                //NOLINT
#include "osal_fs.h"             //NOLINT
#include "osal_socket.h"         //NOLINT

// PSDK wrapper interfaces
#include "psdk_interfaces/msg/aircraft_status.hpp"
#include "psdk_interfaces/msg/altitude.hpp"
#include "psdk_interfaces/msg/battery.hpp"
#include "psdk_interfaces/msg/flight_anomaly.hpp"
#include "psdk_interfaces/msg/flight_status.hpp"
#include "psdk_interfaces/msg/gimbal_rotation.hpp"
#include "psdk_interfaces/msg/gimbal_status.hpp"
#include "psdk_interfaces/msg/gps_details.hpp"
#include "psdk_interfaces/msg/gps_fused.hpp"
#include "psdk_interfaces/msg/home_position.hpp"
#include "psdk_interfaces/msg/position_fused.hpp"
#include "psdk_interfaces/msg/relative_obstacle_info.hpp"
#include "psdk_interfaces/msg/rtk_yaw.hpp"
#include "psdk_interfaces/srv/camera_delete_file_by_index.hpp"
#include "psdk_interfaces/srv/camera_download_file_by_index.hpp"
#include "psdk_interfaces/srv/camera_download_file_list.hpp"
#include "psdk_interfaces/srv/camera_get_ev.hpp"
#include "psdk_interfaces/srv/camera_get_focus_mode.hpp"
#include "psdk_interfaces/srv/camera_get_focus_target.hpp"
#include "psdk_interfaces/srv/camera_get_iso.hpp"
#include "psdk_interfaces/srv/camera_get_laser_ranging_info.hpp"
#include "psdk_interfaces/srv/camera_get_optical_zoom.hpp"
#include "psdk_interfaces/srv/camera_get_shutter_speed.hpp"
#include "psdk_interfaces/srv/camera_get_type.hpp"
#include "psdk_interfaces/srv/camera_record_video.hpp"
#include "psdk_interfaces/srv/camera_set_ev.hpp"
#include "psdk_interfaces/srv/camera_set_focus_mode.hpp"
#include "psdk_interfaces/srv/camera_set_focus_target.hpp"
#include "psdk_interfaces/srv/camera_set_infrared_zoom.hpp"
#include "psdk_interfaces/srv/camera_set_iso.hpp"
#include "psdk_interfaces/srv/camera_set_optical_zoom.hpp"
#include "psdk_interfaces/srv/camera_set_shutter_speed.hpp"
#include "psdk_interfaces/srv/camera_start_shoot_aeb_photo.hpp"
#include "psdk_interfaces/srv/camera_start_shoot_burst_photo.hpp"
#include "psdk_interfaces/srv/camera_start_shoot_interval_photo.hpp"
#include "psdk_interfaces/srv/camera_start_shoot_single_photo.hpp"
#include "psdk_interfaces/srv/camera_stop_shoot_photo.hpp"
#include "psdk_interfaces/srv/get_home_altitude.hpp"
#include "psdk_interfaces/srv/get_obstacle_avoidance.hpp"
#include "psdk_interfaces/srv/gimbal_reset.hpp"
#include "psdk_interfaces/srv/gimbal_set_mode.hpp"
#include "psdk_interfaces/srv/set_home_altitude.hpp"
#include "psdk_interfaces/srv/set_home_from_gps.hpp"
#include "psdk_interfaces/srv/set_obstacle_avoidance.hpp"
#include "psdk_wrapper/psdk_wrapper_utils.hpp"

namespace psdk_ros2
{
/**
 * @class psdk_ros2::PSDKWrapper
 * @brief A ROS wrapper that brings all the DJI PSDK functionalities to ROS
 */

class PSDKWrapper : public rclcpp_lifecycle::LifecycleNode
{
 public:
  using CallbackReturn =
      rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  using SetHomeFromGPS = psdk_interfaces::srv::SetHomeFromGPS;
  using Trigger = std_srvs::srv::Trigger;
  using SetHomeAltitude = psdk_interfaces::srv::SetHomeAltitude;
  using GetHomeAltitude = psdk_interfaces::srv::GetHomeAltitude;
  using SetObstacleAvoidance = psdk_interfaces::srv::SetObstacleAvoidance;
  using GetObstacleAvoidance = psdk_interfaces::srv::GetObstacleAvoidance;
  // Camera
  using CameraStartShootSinglePhoto =
      psdk_interfaces::srv::CameraStartShootSinglePhoto;
  using CameraStartShootBurstPhoto =
      psdk_interfaces::srv::CameraStartShootBurstPhoto;
  using CameraStartShootAEBPhoto =
      psdk_interfaces::srv::CameraStartShootAEBPhoto;
  using CameraStartShootIntervalPhoto =
      psdk_interfaces::srv::CameraStartShootIntervalPhoto;
  using CameraStopShootPhoto = psdk_interfaces::srv::CameraStopShootPhoto;
  using CameraRecordVideo = psdk_interfaces::srv::CameraRecordVideo;
  using CameraGetLaserRangingInfo =
      psdk_interfaces::srv::CameraGetLaserRangingInfo;
  using CameraDownloadFileList = psdk_interfaces::srv::CameraDownloadFileList;
  using CameraDownloadFileByIndex =
      psdk_interfaces::srv::CameraDownloadFileByIndex;
  using CameraDeleteFileByIndex = psdk_interfaces::srv::CameraDeleteFileByIndex;
  using CameraGetType = psdk_interfaces::srv::CameraGetType;
  using CameraSetEV = psdk_interfaces::srv::CameraSetEV;
  using CameraGetEV = psdk_interfaces::srv::CameraGetEV;
  using CameraSetShutterSpeed = psdk_interfaces::srv::CameraSetShutterSpeed;
  using CameraGetShutterSpeed = psdk_interfaces::srv::CameraGetShutterSpeed;
  using CameraSetISO = psdk_interfaces::srv::CameraSetISO;
  using CameraGetISO = psdk_interfaces::srv::CameraGetISO;
  using CameraSetFocusTarget = psdk_interfaces::srv::CameraSetFocusTarget;
  using CameraGetFocusTarget = psdk_interfaces::srv::CameraGetFocusTarget;
  using CameraSetFocusMode = psdk_interfaces::srv::CameraSetFocusMode;
  using CameraGetFocusMode = psdk_interfaces::srv::CameraGetFocusMode;
  using CameraSetOpticalZoom = psdk_interfaces::srv::CameraSetOpticalZoom;
  using CameraGetOpticalZoom = psdk_interfaces::srv::CameraGetOpticalZoom;
  using CameraSetInfraredZoom = psdk_interfaces::srv::CameraSetInfraredZoom;
  // Gimbal
  using GimbalSetMode = psdk_interfaces::srv::GimbalSetMode;
  using GimbalReset = psdk_interfaces::srv::GimbalReset;

  /**
   * @brief Construct a new PSDKWrapper object
   *
   * @param node_name
   */
  explicit PSDKWrapper(const std::string& node_name);

  /**
   * @brief Destroy the PSDKWrapper object
   *
   */
  ~PSDKWrapper();

 protected:
  /**
   * @brief Configures member variable and sets the environment
   * @param state Reference to Lifecycle state
   * @return CallbackReturn SUCCESS or FAILURE
   */
  CallbackReturn on_configure(const rclcpp_lifecycle::State& state) override;

  /**
   * @brief Initializes main PSDK modules
   * @param state Reference to Lifecycle state
   * @return CallbackReturn SUCCESS or FAILURE
   */
  CallbackReturn on_activate(const rclcpp_lifecycle::State& state) override;

  /**
   * @brief Deactivates main PSDK modules and other member variables
   * @param state Reference to Lifecycle state
   * @return CallbackReturn SUCCESS or FAILURE
   */
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State& state) override;

  /**
   * @brief Resets member variables
   * @param state Reference to Lifecycle state
   * @return CallbackReturn SUCCESS or FAILURE
   */
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State& state) override;

  /**
   * @brief Initializes main PSDK modules
   * @param state Reference to Lifecycle state
   * @return CallbackReturn SUCCESS or FAILURE
   */
  CallbackReturn on_shutdown(const rclcpp_lifecycle::State& state) override;

 private:
  struct PSDKParams
  {
    std::string app_name;
    std::string app_id;
    std::string app_key;
    std::string app_license;
    std::string developer_account;
    std::string baudrate;
    std::string hardware_connection;
    std::string uart_dev_1;
    std::string uart_dev_2;
    std::string imu_frame;
    std::string body_frame;
    std::string map_frame;
    std::string gimbal_frame;
    int imu_frequency;
    int attitude_frequency;
    int acceleration_frequency;
    int velocity_frequency;
    int angular_velocity_frequency;
    int position_frequency;
    int gps_data_frequency;
    int rtk_data_frequency;
    int magnetometer_frequency;
    int rc_channels_data_frequency;
    int gimbal_data_frequency;
    int flight_status_frequency;
    int battery_level_frequency;
    int control_information_frequency;
  };

  /**
   * @brief Set the environment handlers
   * @return true/false
   */
  bool set_environment();

  /**
   * @brief Set the user application information.
   * @param user_info object containing the main information regarding the psdk
   * application
   * @return true/false
   */
  bool set_user_info(T_DjiUserInfo* user_info);

  /**
   * @brief Load ROS parameters
   *
   */
  void load_parameters();

  /**
   * @brief Initiate the PSDK application
   * @param user_info object containing the main information regarding the psdk
   * application
   * @return true/false
   */
  bool init(T_DjiUserInfo* user_info);

  /**
   * @brief Initiate the telemetry module
   * @return true/false
   */
  bool init_telemetry();

  /**
   * @brief Initiate the flight control module
   * @return true/false
   */
  bool init_flight_control();

  /**
   * @brief Initiate the camera module
   * @return true/false
   */
  bool init_camera_manager();
  /**
   * @brief Deinitiate the camera module
   * @return true/false
   */
  bool deinit_camera_manager();
  /**
   * @brief Initiate the gimbal module
   * @return true/false
   */
  bool init_gimbal_manager();
  /**
   * @brief Denitiate the gimbal module
   * @return true/false
   */
  bool deinit_gimbal_manager();

  /**
   * @brief Get the DJI frequency object associated with a certain frequency
   * @param frequency
   * @return E_DjiDataSubscriptionTopicFreq
   */
  E_DjiDataSubscriptionTopicFreq get_frequency(const int frequency);

  /**
   * @brief Initializes all ROS elements (e.g. subscribers, publishers,
   * services)
   */
  void initialize_ros_elements();

  /**
   * @brief Activates all ROS elements
   */
  void activate_ros_elements();

  /**
   * @brief Deactivates all ROS elements
   */
  void deactivate_ros_elements();

  /**
   * @brief Cleans all ROS elements
   */
  void clean_ros_elements();
  /**
   * @brief Subscribe to DJI topics
   */
  void subscribe_psdk_topics();

  /**
   * @brief Unsubscribe to DJI topics
   */
  void unsubscribe_psdk_topics();

  /* C-typed DJI topic subscriber callbacks*/
  friend T_DjiReturnCode c_attitude_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_velocity_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_imu_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_position_vo_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_fused_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_position_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_velocity_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_details_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_signal_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gps_control_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rtk_position_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rtk_velocity_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rtk_yaw_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rtk_position_info_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rtk_yaw_info_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_magnetometer_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_rc_callback(const uint8_t* data, uint16_t dataSize,
                                       const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gimbal_angles_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_gimbal_status_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_flight_status_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_display_mode_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_landing_gear_status_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_motor_start_error_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_flight_anomaly_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_battery_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  friend T_DjiReturnCode c_height_fused_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);

  /*C++ type DJI topic subscriber callbacks*/
  T_DjiReturnCode attitude_callback(const uint8_t* data, uint16_t dataSize,
                                    const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode velocity_callback(const uint8_t* data, uint16_t dataSize,
                                    const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode imu_callback(const uint8_t* data, uint16_t dataSize,
                               const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode position_vo_callback(const uint8_t* data, uint16_t dataSize,
                                       const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_fused_callback(const uint8_t* data, uint16_t dataSize,
                                     const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_position_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_velocity_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_details_callback(const uint8_t* data, uint16_t dataSize,
                                       const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_signal_callback(const uint8_t* data, uint16_t dataSize,
                                      const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gps_control_callback(const uint8_t* data, uint16_t dataSize,
                                       const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rtk_position_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rtk_velocity_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rtk_yaw_callback(const uint8_t* data, uint16_t dataSize,
                                   const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rtk_position_info_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rtk_yaw_info_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode magnetometer_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode rc_callback(const uint8_t* data, uint16_t dataSize,
                              const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gimbal_angles_callback(const uint8_t* data, uint16_t dataSize,
                                         const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode gimbal_status_callback(const uint8_t* data, uint16_t dataSize,
                                         const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode flight_status_callback(const uint8_t* data, uint16_t dataSize,
                                         const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode display_mode_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode landing_gear_status_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode motor_start_error_callback(
      const uint8_t* data, uint16_t dataSize,
      const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode flight_anomaly_callback(const uint8_t* data,
                                          uint16_t dataSize,
                                          const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode battery_callback(const uint8_t* data, uint16_t dataSize,
                                   const T_DjiDataTimestamp* timestamp);
  T_DjiReturnCode height_fused_callback(const uint8_t* data, uint16_t dataSize,
                                        const T_DjiDataTimestamp* timestamp);

  /* ROS subscriber callbacks*/
  /**
   * @brief Callback function to control aircraft position and yaw. This
   * function uses the ground reference frame.
   * @param msg  sensor_msgs::msg::Joy. Axes represent the x, y, z and yaw
   * command.
   */
  void flight_control_position_yaw_cb(
      const sensor_msgs::msg::Joy::SharedPtr msg);
  /**
   * @brief Callback function to control aircraft velocity and yaw rate. This
   * function uses the ground reference frame.
   * @param msg  sensor_msgs::msg::Joy. Axes represent the x, y, z and yaw
   * command.
   */
  void flight_control_velocity_yawrate_cb(
      const sensor_msgs::msg::Joy::SharedPtr msg);

  /**
   * @brief Callback function to control aircraft velocity and yaw. This
   * function uses the body reference frame.
   * @param msg  sensor_msgs::msg::Joy. Axes represent the x, y, z and yaw
   * command.
   */
  void flight_control_body_velocity_yawrate_cb(
      const sensor_msgs::msg::Joy::SharedPtr msg);

  /**
   * @brief Callback function to control roll, pitch, yawrate and thrust. This
   * function uses the body reference frame.
   * @param msg  sensor_msgs::msg::Joy. Axes represent the x, y, z and yaw
   * command.
   */
  void flight_control_rollpitch_yawrate_vertpos_cb(
      const sensor_msgs::msg::Joy::SharedPtr msg);

  /**
   * @brief Callback function to exposing a generic control method of the
   * aircraft.The type of commands as well as the reference frame is specified
   * in a flag within the msg.
   * @note This type of control is not implemented at this moment.
   * @param msg  sensor_msgs::msg::Joy. Axes represent the x, y, z and yaw
   * command.
   */
  void flight_control_generic_cb(const sensor_msgs::msg::Joy::SharedPtr msg);

  /**
   * @brief Callback function to control roll, pitch, yaw and time.
   * @param msg  psdk_interfaces::msg::GimbalRotation.
   * Rotation mode allows to set incremental, absolute or speed mode
   * command.
   */
  void gimbal_rotation_cb(
      const psdk_interfaces::msg::GimbalRotation::SharedPtr msg);

  /* ROS Service callbacks*/
  void set_home_from_gps_cb(
      const std::shared_ptr<SetHomeFromGPS::Request> request,
      const std::shared_ptr<SetHomeFromGPS::Response> response);
  void set_home_from_current_location_cb(
      const std::shared_ptr<Trigger::Request> request,
      const std::shared_ptr<Trigger::Response> response);
  void set_home_altitude_cb(
      const std::shared_ptr<SetHomeAltitude::Request> request,
      const std::shared_ptr<SetHomeAltitude::Response> response);
  void get_home_altitude_cb(
      const std::shared_ptr<GetHomeAltitude::Request> request,
      const std::shared_ptr<GetHomeAltitude::Response> response);
  void start_go_home_cb(const std::shared_ptr<Trigger::Request> request,
                        const std::shared_ptr<Trigger::Response> response);
  void cancel_go_home_cb(const std::shared_ptr<Trigger::Request> request,
                         const std::shared_ptr<Trigger::Response> response);
  void obtain_ctrl_authority_cb(
      const std::shared_ptr<Trigger::Request> request,
      const std::shared_ptr<Trigger::Response> response);
  void release_ctrl_authority_cb(
      const std::shared_ptr<Trigger::Request> request,
      const std::shared_ptr<Trigger::Response> response);
  void turn_on_motors_cb(const std::shared_ptr<Trigger::Request> request,
                         const std::shared_ptr<Trigger::Response> response);
  void turn_off_motors_cb(const std::shared_ptr<Trigger::Request> request,
                          const std::shared_ptr<Trigger::Response> response);
  void start_takeoff_cb(const std::shared_ptr<Trigger::Request> request,
                        const std::shared_ptr<Trigger::Response> response);
  void start_landing_cb(const std::shared_ptr<Trigger::Request> request,
                        const std::shared_ptr<Trigger::Response> response);
  void cancel_landing_cb(const std::shared_ptr<Trigger::Request> request,
                         const std::shared_ptr<Trigger::Response> response);
  void start_confirm_landing_cb(
      const std::shared_ptr<Trigger::Request> request,
      const std::shared_ptr<Trigger::Response> response);
  void start_force_landing_cb(
      const std::shared_ptr<Trigger::Request> request,
      const std::shared_ptr<Trigger::Response> response);
  void set_horizontal_vo_obstacle_avoidance_cb(
      const std::shared_ptr<SetObstacleAvoidance::Request> request,
      const std::shared_ptr<SetObstacleAvoidance::Response> response);
  void set_horizontal_radar_obstacle_avoidance_cb(
      const std::shared_ptr<SetObstacleAvoidance::Request> request,
      const std::shared_ptr<SetObstacleAvoidance::Response> response);
  void set_upwards_vo_obstacle_avoidance_cb(
      const std::shared_ptr<SetObstacleAvoidance::Request> request,
      const std::shared_ptr<SetObstacleAvoidance::Response> response);
  void set_upwards_radar_obstacle_avoidance_cb(
      const std::shared_ptr<SetObstacleAvoidance::Request> request,
      const std::shared_ptr<SetObstacleAvoidance::Response> response);
  void set_downwards_vo_obstacle_avoidance_cb(
      const std::shared_ptr<SetObstacleAvoidance::Request> request,
      const std::shared_ptr<SetObstacleAvoidance::Response> response);
  void get_horizontal_vo_obstacle_avoidance_cb(
      const std::shared_ptr<GetObstacleAvoidance::Request> request,
      const std::shared_ptr<GetObstacleAvoidance::Response> response);
  void get_horizontal_radar_obstacle_avoidance_cb(
      const std::shared_ptr<GetObstacleAvoidance::Request> request,
      const std::shared_ptr<GetObstacleAvoidance::Response> response);
  void get_downwards_vo_obstacle_avoidance_cb(
      const std::shared_ptr<GetObstacleAvoidance::Request> request,
      const std::shared_ptr<GetObstacleAvoidance::Response> response);
  void get_upwards_vo_obstacle_avoidance_cb(
      const std::shared_ptr<GetObstacleAvoidance::Request> request,
      const std::shared_ptr<GetObstacleAvoidance::Response> response);
  void get_upwards_radar_obstacle_avoidance_cb(
      const std::shared_ptr<GetObstacleAvoidance::Request> request,
      const std::shared_ptr<GetObstacleAvoidance::Response> response);
  // Camera
  void camera_get_type_cb(
      const std::shared_ptr<CameraGetType::Request> request,
      const std::shared_ptr<CameraGetType::Response> response);
  void camera_set_ev_cb(const std::shared_ptr<CameraSetEV::Request> request,
                        const std::shared_ptr<CameraSetEV::Response> response);
  void camera_get_ev_cb(const std::shared_ptr<CameraGetEV::Request> request,
                        const std::shared_ptr<CameraGetEV::Response> response);
  void camera_set_shutter_speed_cb(
      const std::shared_ptr<CameraSetShutterSpeed::Request> request,
      const std::shared_ptr<CameraSetShutterSpeed::Response> response);
  void camera_get_shutter_speed_cb(
      const std::shared_ptr<CameraGetShutterSpeed::Request> request,
      const std::shared_ptr<CameraGetShutterSpeed::Response> response);
  void camera_set_iso_cb(
      const std::shared_ptr<CameraSetISO::Request> request,
      const std::shared_ptr<CameraSetISO::Response> response);
  void camera_get_iso_cb(
      const std::shared_ptr<CameraGetISO::Request> request,
      const std::shared_ptr<CameraGetISO::Response> response);
  void camera_set_focus_target_cb(
      const std::shared_ptr<CameraSetFocusTarget::Request> request,
      const std::shared_ptr<CameraSetFocusTarget::Response> response);
  void camera_get_focus_target_cb(
      const std::shared_ptr<CameraGetFocusTarget::Request> request,
      const std::shared_ptr<CameraGetFocusTarget::Response> response);
  void camera_set_focus_mode_cb(
      const std::shared_ptr<CameraSetFocusMode::Request> request,
      const std::shared_ptr<CameraSetFocusMode::Response> response);
  void camera_get_focus_mode_cb(
      const std::shared_ptr<CameraGetFocusMode::Request> request,
      const std::shared_ptr<CameraGetFocusMode::Response> response);
  void camera_set_optical_zoom_cb(
      const std::shared_ptr<CameraSetOpticalZoom::Request> request,
      const std::shared_ptr<CameraSetOpticalZoom::Response> response);
  void camera_get_optical_zoom_cb(
      const std::shared_ptr<CameraGetOpticalZoom::Request> request,
      const std::shared_ptr<CameraGetOpticalZoom::Response> response);
  void camera_set_infrared_zoom_cb(
      const std::shared_ptr<CameraSetInfraredZoom::Request> request,
      const std::shared_ptr<CameraSetInfraredZoom::Response> response);
  void camera_start_shoot_single_photo_cb(
      const std::shared_ptr<CameraStartShootSinglePhoto::Request> request,
      const std::shared_ptr<CameraStartShootSinglePhoto::Response> response);
  void camera_start_shoot_burst_photo_cb(
      const std::shared_ptr<CameraStartShootBurstPhoto::Request> request,
      const std::shared_ptr<CameraStartShootBurstPhoto::Response> response);
  void camera_start_shoot_aeb_photo_cb(
      const std::shared_ptr<CameraStartShootAEBPhoto::Request> request,
      const std::shared_ptr<CameraStartShootAEBPhoto::Response> response);
  void camera_start_shoot_interval_photo_cb(
      const std::shared_ptr<CameraStartShootIntervalPhoto::Request> request,
      const std::shared_ptr<CameraStartShootIntervalPhoto::Response> response);
  void camera_stop_shoot_photo_cb(
      const std::shared_ptr<CameraStopShootPhoto::Request> request,
      const std::shared_ptr<CameraStopShootPhoto::Response> response);
  void camera_record_video_cb(
      const std::shared_ptr<CameraRecordVideo::Request> request,
      const std::shared_ptr<CameraRecordVideo::Response> response);
  void camera_get_laser_ranging_info_cb(
      const std::shared_ptr<CameraGetLaserRangingInfo::Request> request,
      const std::shared_ptr<CameraGetLaserRangingInfo::Response> response);
  void camera_download_file_list_cb(
      const std::shared_ptr<CameraDownloadFileList::Request> request,
      const std::shared_ptr<CameraDownloadFileList::Response> response);
  void camera_download_file_by_index_cb(
      const std::shared_ptr<CameraDownloadFileByIndex::Request> request,
      const std::shared_ptr<CameraDownloadFileByIndex::Response> response);
  void camera_delete_file_by_index_cb(
      const std::shared_ptr<CameraDeleteFileByIndex::Request> request,
      const std::shared_ptr<CameraDeleteFileByIndex::Response> response);
  void gimbal_set_mode_cb(
      const std::shared_ptr<GimbalSetMode::Request> request,
      const std::shared_ptr<GimbalSetMode::Response> response);
  void gimbal_reset_cb(const std::shared_ptr<GimbalReset::Request> request,
                       const std::shared_ptr<GimbalReset::Response> response);

  /* ROS Publishers */
  rclcpp_lifecycle::LifecyclePublisher<
      geometry_msgs::msg::QuaternionStamped>::SharedPtr attitude_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      geometry_msgs::msg::TwistStamped>::SharedPtr velocity_ground_pub_;
  rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::Imu>::SharedPtr
      imu_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::PositionFused>::SharedPtr position_fused_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::GPSFused>::SharedPtr gps_fused_pub_;
  rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::NavSatFix>::SharedPtr
      gps_position_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      geometry_msgs::msg::TwistStamped>::SharedPtr gps_velocity_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::GPSDetails>::SharedPtr gps_details_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt8>::SharedPtr
      gps_signal_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt8>::SharedPtr
      gps_control_pub_;
  rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::NavSatFix>::SharedPtr
      rtk_position_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      geometry_msgs::msg::TwistStamped>::SharedPtr rtk_velocity_pub_;
  rclcpp_lifecycle::LifecyclePublisher<psdk_interfaces::msg::RTKYaw>::SharedPtr
      rtk_yaw_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt8>::SharedPtr
      rtk_position_info_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt8>::SharedPtr
      rtk_yaw_info_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      sensor_msgs::msg::MagneticField>::SharedPtr magnetic_field_pub_;
  rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::Joy>::SharedPtr
      rc_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      geometry_msgs::msg::Vector3Stamped>::SharedPtr gimbal_angles_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::GimbalStatus>::SharedPtr gimbal_status_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::FlightStatus>::SharedPtr flight_status_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt8>::SharedPtr
      landing_gear_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::UInt16>::SharedPtr
      motor_start_error_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::AircraftStatus>::SharedPtr aircraft_status_pub_;
  rclcpp_lifecycle::LifecyclePublisher<
      psdk_interfaces::msg::FlightAnomaly>::SharedPtr flight_anomaly_pub_;
  rclcpp_lifecycle::LifecyclePublisher<psdk_interfaces::msg::Battery>::SharedPtr
      battery_pub_;
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::Float32>::SharedPtr
      height_fused_pub_;

  /* ROS subscribers*/
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr
      flight_control_generic_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr
      flight_control_position_yaw_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr
      flight_control_velocity_yawrate_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr
      flight_control_body_velocity_yawrate_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr
      flight_control_rollpitch_yawrate_vertpos_sub_;
  // Gimbal
  rclcpp::Subscription<psdk_interfaces::msg::GimbalRotation>::SharedPtr
      gimbal_rotation_sub_;

  /* ROS Services */
  rclcpp::Service<SetHomeFromGPS>::SharedPtr set_home_from_gps_srv_;
  rclcpp::Service<Trigger>::SharedPtr set_home_from_current_location_srv_;
  rclcpp::Service<SetHomeAltitude>::SharedPtr set_home_altitude_srv_;
  rclcpp::Service<GetHomeAltitude>::SharedPtr get_home_altitude_srv_;
  rclcpp::Service<Trigger>::SharedPtr start_go_home_srv_;
  rclcpp::Service<Trigger>::SharedPtr cancel_go_home_srv_;
  rclcpp::Service<Trigger>::SharedPtr obtain_ctrl_authority_srv_;
  rclcpp::Service<Trigger>::SharedPtr release_ctrl_authority_srv_;
  rclcpp::Service<Trigger>::SharedPtr turn_on_motors_srv_;
  rclcpp::Service<Trigger>::SharedPtr turn_off_motors_srv_;
  rclcpp::Service<Trigger>::SharedPtr takeoff_srv_;
  rclcpp::Service<Trigger>::SharedPtr land_srv_;
  rclcpp::Service<Trigger>::SharedPtr cancel_landing_srv_;
  rclcpp::Service<Trigger>::SharedPtr start_confirm_landing_srv_;
  rclcpp::Service<Trigger>::SharedPtr start_force_landing_srv_;
  rclcpp::Service<SetObstacleAvoidance>::SharedPtr
      set_horizontal_vo_obstacle_avoidance_srv_;
  rclcpp::Service<SetObstacleAvoidance>::SharedPtr
      set_horizontal_radar_obstacle_avoidance_srv_;
  rclcpp::Service<SetObstacleAvoidance>::SharedPtr
      set_upwards_vo_obstacle_avoidance_srv_;
  rclcpp::Service<SetObstacleAvoidance>::SharedPtr
      set_upwards_radar_obstacle_avoidance_srv_;
  rclcpp::Service<SetObstacleAvoidance>::SharedPtr
      set_downwards_vo_obstacle_avoidance_srv_;
  rclcpp::Service<GetObstacleAvoidance>::SharedPtr
      get_horizontal_vo_obstacle_avoidance_srv_;
  rclcpp::Service<GetObstacleAvoidance>::SharedPtr
      get_upwards_vo_obstacle_avoidance_srv_;
  rclcpp::Service<GetObstacleAvoidance>::SharedPtr
      get_upwards_radar_obstacle_avoidance_srv_;
  rclcpp::Service<GetObstacleAvoidance>::SharedPtr
      get_downwards_vo_obstacle_avoidance_srv_;
  rclcpp::Service<GetObstacleAvoidance>::SharedPtr
      get_horizontal_radar_obstacle_avoidance_srv_;
  // Camera
  rclcpp::Service<CameraStartShootSinglePhoto>::SharedPtr
      camera_start_shoot_single_photo_service_;
  rclcpp::Service<CameraStartShootBurstPhoto>::SharedPtr
      camera_start_shoot_burst_photo_service_;
  rclcpp::Service<CameraStartShootAEBPhoto>::SharedPtr
      camera_start_shoot_aeb_photo_service_;
  rclcpp::Service<CameraStartShootIntervalPhoto>::SharedPtr
      camera_start_shoot_interval_photo_service_;
  rclcpp::Service<CameraStopShootPhoto>::SharedPtr
      camera_stop_shoot_photo_service_;
  rclcpp::Service<CameraRecordVideo>::SharedPtr camera_record_video_service_;
  rclcpp::Service<CameraGetLaserRangingInfo>::SharedPtr
      camera_get_laser_ranging_info_service_;
  rclcpp::Service<CameraDownloadFileList>::SharedPtr
      camera_download_file_list_service_;
  rclcpp::Service<CameraDownloadFileByIndex>::SharedPtr
      camera_download_file_by_index_service_;
  rclcpp::Service<CameraDeleteFileByIndex>::SharedPtr
      camera_delete_file_by_index_service_;
  rclcpp::Service<CameraGetType>::SharedPtr camera_get_type_service_;
  rclcpp::Service<CameraSetEV>::SharedPtr camera_set_ev_service_;
  rclcpp::Service<CameraGetEV>::SharedPtr camera_get_ev_service_;
  rclcpp::Service<CameraSetShutterSpeed>::SharedPtr
      camera_set_shutter_speed_service_;
  rclcpp::Service<CameraGetShutterSpeed>::SharedPtr
      camera_get_shutter_speed_service_;
  rclcpp::Service<CameraSetISO>::SharedPtr camera_set_iso_service_;
  rclcpp::Service<CameraGetISO>::SharedPtr camera_get_iso_service_;
  rclcpp::Service<CameraSetFocusTarget>::SharedPtr
      camera_set_focus_target_service_;
  rclcpp::Service<CameraGetFocusTarget>::SharedPtr
      camera_get_focus_target_service_;
  rclcpp::Service<CameraSetFocusMode>::SharedPtr camera_set_focus_mode_service_;
  rclcpp::Service<CameraGetFocusMode>::SharedPtr camera_get_focus_mode_service_;
  rclcpp::Service<CameraSetOpticalZoom>::SharedPtr
      camera_set_optical_zoom_service_;
  rclcpp::Service<CameraGetOpticalZoom>::SharedPtr
      camera_get_optical_zoom_service_;
  rclcpp::Service<CameraSetInfraredZoom>::SharedPtr
      camera_set_infrared_zoom_service_;
  // Gimbal
  rclcpp::Service<GimbalSetMode>::SharedPtr gimbal_set_mode_service_;
  rclcpp::Service<GimbalReset>::SharedPtr gimbal_reset_service_;

  /**
   * @brief Get the gps signal level
   * @return int gps signal level
   */
  inline int
  get_gps_signal_level()
  {
    return gps_signal_level_;
  };

  /**
   * @brief Set the gps signal level object
   *
   * @param gps_signal
   */
  inline void
  set_gps_signal_level(const int gps_signal)
  {
    gps_signal_level_ = gps_signal;
  };

  /**
   * @brief Checks if local altitude reference is set or not
   * @return true if set, false otherwise
   */
  inline bool
  is_local_altitude_reference_set()
  {
    return local_altitude_reference_set_;
  };
  /**
   * @brief Get the local altitude reference value. If it is not set, default
   * value is 0
   * @return float local_altitude_reference
   */
  inline float
  get_local_altitude_reference()
  {
    return local_altitude_reference_;
  };

  /**
   * @brief Sets the local altitude reference object
   * @param altitude  value to which to set the local altitude reference
   */
  void set_local_altitude_reference(const float altitude);

  /* Global variables*/
  PSDKParams params_;
  rclcpp::Node::SharedPtr node_;

  int gps_signal_level_{0};
  float local_altitude_reference_{0};
  bool local_altitude_reference_set_{false};

  const rmw_qos_profile_t& qos_profile_{rmw_qos_profile_services_default};

  std::map<E_DjiCameraType, std::string> camera_type_str = {
      {DJI_CAMERA_TYPE_UNKNOWN, "Unkown"},
      {DJI_CAMERA_TYPE_Z30, "Zenmuse Z30"},
      {DJI_CAMERA_TYPE_XT2, "Zenmuse XT2"},
      {DJI_CAMERA_TYPE_PSDK, "Payload Camera"},
      {DJI_CAMERA_TYPE_XTS, "Zenmuse XTS"},
      {DJI_CAMERA_TYPE_H20, "Zenmuse H20"},
      {DJI_CAMERA_TYPE_H20T, "Zenmuse H20T"},
      {DJI_CAMERA_TYPE_P1, "Zenmuse P1"},
      {DJI_CAMERA_TYPE_L1, "Zenmuse L1"},
      {DJI_CAMERA_TYPE_H20N, "Zenmuse H20N"},
      {DJI_CAMERA_TYPE_M30, "M30 Camera"},
      {DJI_CAMERA_TYPE_M30T, "M30T Camera"},
      {DJI_CAMERA_TYPE_M3E, "M3E Camera"},
      {DJI_CAMERA_TYPE_M3T, "M3T Camera"},
  };
};

/**
 * @brief Global pointer to the class object. It is initialized in the main.cpp
 * file. This pointer is needed to access member functions from non-member
 * functions, such as the C-type callbacks
 */
extern std::shared_ptr<PSDKWrapper> global_ptr_;
}  // namespace psdk_ros2

#endif  // PSDK_WRAPPER_INCLUDE_PSDK_WRAPPER_PSDK_WRAPPER_HPP_
