// bsd license blah blah
#include <cstring>
#include <ros/ros.h>
#include <sensor_msgs/JointState.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <tf_conversions/tf_kdl.h>
#include "openarms/ArmSensors.h"
#include "wiimote/State.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <Eigen/LU>
#include "robot_state_publisher/treefksolverposfull_recursive.hpp"
#include <string>
#include <map>
#include <kdl_parser/kdl_parser.hpp>
using std::string;
using std::map;
using std::make_pair;
using namespace Eigen;

// (MQ:) much of the interface code between ROS and KDL in this code is lifted
// straight from Wim Meeussen's robot_state_publisher

class EKF
{
public:
  static const int NUM_JOINTS = 4;
  static const int N = NUM_JOINTS * 2; // joint positions and velocities
  static const int K = 12; // 2 wiimotes, 6-dof each
  VectorXd mean;
  MatrixXd cov, transition_cov, meas_cov;
  //tf::TransformListener tf_listener;
  tf::StampedTransform upperarm_tf, lowerarm_tf;
  KDL::Tree *tree;
  boost::scoped_ptr<KDL::TreeFkSolverPosFull_recursive> fk_solver;
  std::string tree_root_name;

  EKF(KDL::Tree *init_tree) : tree(init_tree)
  {
    fk_solver.reset(new KDL::TreeFkSolverPosFull_recursive(*tree));
    KDL::SegmentMap::const_iterator root_seg = tree->getRootSegment();
    tree_root_name = root_seg->first;
  }

  void init(const VectorXd &init_mean, const MatrixXd &init_cov,
            const MatrixXd &init_transition_cov,
            const MatrixXd &init_meas_cov)
  {
    mean = init_mean;
    cov = init_cov;
    transition_cov = init_transition_cov;
    meas_cov = init_meas_cov;
  }
  Matrix4d construct_transform(const Matrix3d &rot, const Vector3d &vec)
  {
    Matrix4d T_matrix;
    T_matrix.block(0, 0, 3, 3) = rot;
    T_matrix.block(0, 3, 3, 1) = vec;
    T_matrix(3, 0) = T_matrix(3, 1) = T_matrix(3, 2) = 0;
    T_matrix(3, 3) = 1;
    return T_matrix;
  }
  VectorXd f(const VectorXd &x) // state transition
  {
    const double SENSOR_RATE = 66.667; // appears to be what the wiimote gives
    VectorXd x_f(N);
    for (int i = 0; i < NUM_JOINTS; i++)
    {
      x_f[i] = x[i] + 1. / SENSOR_RATE * x[i+NUM_JOINTS]; // newton was genius
      x_f[i] *= 0.99; // decay to zero 
    }
    for (int i = NUM_JOINTS; i < 2*NUM_JOINTS; i++)
      x_f[i] = x[i]; // assume constant velocity
    return x_f;
  }
  bool update_tf()
  {
    /*
    try
    {
      tf_listener.lookupTransform("/upperarm_wiimote_link", "/world",
                                  ros::Time(0), upperarm_tf);
      tf_listener.lookupTransform("/lowerarm_wiimote_link", "/world",
                                  ros::Time(0), lowerarm_tf);
    }
    catch (tf::TransformException ex)
    {
      ROS_ERROR("%s", ex.what());
      return false;
    }
    btVector3 accel = 9.81 * upperarm_tf.getBasis().getColumn(2);
    ROS_INFO("predicted upperarm: %+05.2f %+05.2f %+05.2f\n",
             accel.x(), accel.y(), accel.z());
    return true;
    */
    return true;
  }
  VectorXd z(const VectorXd &x)
  {
    map<string, double> joint_pos;
    joint_pos.insert(make_pair("shoulder_flexion", x[0]));
    joint_pos.insert(make_pair("shoulder_abduction", x[1]));
    joint_pos.insert(make_pair("humeral_rotation", x[2]));
    joint_pos.insert(make_pair("elbow", x[3]));
    map<string, KDL::Frame> link_poses;
    fk_solver->JntToCart(joint_pos, link_poses);
    VectorXd pred_sensors = VectorXd::Zero(12);
    if (link_poses.size() < 4)
      ROS_ERROR("couldn't compute link poses.");
    for (map<string, KDL::Frame>::const_iterator f = link_poses.begin();
         f != link_poses.end(); ++f)
    {
      if (f->first == "upperarm_wiimote_link")
      {
        tf::Transform tf_frame;
        tf::TransformKDLToTF(f->second, tf_frame);
        btVector3 accel = tf_frame.getBasis().getRow(2);
        pred_sensors[0] = accel.x();
        pred_sensors[1] = accel.y();
        pred_sensors[2] = accel.z();
        /*
        ROS_INFO("predicted upperarm: %+05.2f %+05.2f %+05.2f",
                 accel.x(), accel.y(), accel.z());
        */
      }
      else if (f->first == "lowerarm_wiimote_link")
      {
        tf::Transform tf_frame;
        tf::TransformKDLToTF(f->second, tf_frame);
        btVector3 accel = tf_frame.getBasis().getRow(2);
        pred_sensors[3] = accel.x();
        pred_sensors[4] = accel.y();
        pred_sensors[5] = accel.z();
        /*
        ROS_INFO("predicted lowerarm: %+05.2f %+05.2f %+05.2f",
                 accel.x(), accel.y(), accel.z());
        */
      }
    }
    return pred_sensors;
  }
  void update(const VectorXd &meas)
  {
    const double DELTA = 0.0001; // numerical derivatives
    // state update ////////////////////////////////////////////////////////
    // predict the next mean
    VectorXd pred_mean = f(mean);
    // predict covariance: compute linearization of transition function at mean
    MatrixXd A(N, N);
    for (int j = 0; j < N; j++)
    {
      VectorXd bumped = mean;
      bumped(j) += DELTA;
      VectorXd bumped_next = f(bumped);
      A.col(j) = (bumped_next - pred_mean) / DELTA;
    }
    MatrixXd pred_cov = A * cov * A.transpose() + transition_cov;
    // measurement update //////////////////////////////////////////////////
    // compute linearization C of the measurement function z at predicted mean
    VectorXd pred_meas = z(pred_mean);
    MatrixXd C(K, N);
    for (int j = 0; j < N; j++)
    {
      VectorXd bumped = pred_mean;
      bumped(j) += DELTA;
      VectorXd bumped_meas = z(bumped);
      C.col(j) = (bumped_meas - pred_meas) / DELTA;
    }
    // compute kalman gain
    MatrixXd t = C * pred_cov * C.transpose() + meas_cov;
    MatrixXd KG = pred_cov * C.transpose() * t.inverse();
    mean = pred_mean + KG * (meas - pred_meas);
    cov = (MatrixXd::Identity(N, N) - KG * C) * pred_cov;
  }
};

ros::Publisher *g_joint_pub = NULL;
tf::TransformBroadcaster *g_tf_broadcaster = NULL;
EKF *g_ekf = NULL;
bool g_wm2_valid = false;
VectorXd g_wm2;

void wiimote2_cb(const wiimote::State::ConstPtr &msg)
{
  g_wm2[0] = msg->linear_acceleration_zeroed.x;
  g_wm2[1] = msg->linear_acceleration_zeroed.y;
  g_wm2[2] = msg->linear_acceleration_zeroed.z;
  g_wm2.normalize();
  g_wm2_valid = true;
}

void wiimote_cb(const wiimote::State::ConstPtr &msg)
{
  if (!g_wm2_valid)
    return;
  /*
  printf("wiimote %+05.2f %+05.2f %+05.2f\n",
         msg->angular_velocity_zeroed.x,
         msg->angular_velocity_zeroed.y,
         msg->angular_velocity_zeroed.z);
  */
  /*
  printf("wiimote accels %+05.2f %+05.2f %+05.2f\n",
         msg->linear_acceleration_zeroed.x,
         msg->linear_acceleration_zeroed.y,
         msg->linear_acceleration_zeroed.z);
  */

  sensor_msgs::JointState master_state;
  master_state.header.stamp = ros::Time::now();
  master_state.name.resize(4);
  master_state.position.resize(4);
  master_state.name[0] = "shoulder_flexion";
  master_state.name[1] = "shoulder_abduction";
  master_state.name[2] = "humeral_rotation";
  master_state.name[3] = "elbow";
  /*
  master_state.position[0] = .3;
  master_state.position[1] = -.6;
  master_state.position[2] = -1.0;
  master_state.position[3] = 0.3;
  */
  /*
  VectorXd x = VectorXd::Zero(8);
  for (int i = 0; i < 4; i++)
    x[i] = master_state.position[i];
  */
  //g_ekf->z(x);
  //if (g_ekf->update_tf())
  VectorXd meas = VectorXd::Zero(12);
  meas[0] = msg->linear_acceleration_zeroed.x;
  meas[1] = msg->linear_acceleration_zeroed.y;
  meas[2] = msg->linear_acceleration_zeroed.z;
  meas.normalize();

  // copy over accelerometers from second wiimote
  for (int i = 0; i < 3; i++)
    meas[i+3] = g_wm2[i];

  printf("\n");
  for (int i = 0; i < 6; i++)
    printf("%+05.2f  ", meas[i]);
  printf("\n");


  g_ekf->update(meas);

  master_state.position[0] = g_ekf->mean[0];
  master_state.position[1] = g_ekf->mean[1];
  master_state.position[2] = g_ekf->mean[2];
  master_state.position[3] = g_ekf->mean[3];
  g_joint_pub->publish(master_state);
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "wiimote_master");
  ros::NodeHandle n;
  ros::NodeHandle n_private("~");
  g_wm2 = VectorXd::Zero(6);

  ///////////////////////////////////////////////////////////////////////
  // this code lifted from the robot_state_publisher package
  string robot_desc;
  n.getParam("master_description", robot_desc);
  //ROS_INFO("robot: %s", robot_desc.c_str());
  KDL::Tree tree;
  if (!kdl_parser::treeFromString(robot_desc, tree))
  {
    ROS_ERROR("failed to extract kdl tree from xml robot description");
    return 1;
  }
  if (!tree.getNrOfSegments())
  {
    ROS_ERROR("empty tree. sad.");
    return 1;
  }
  ROS_INFO("parsed tree successfully");
  ///////////////////////////////////////////////////////////////////////

  //n_private.getParam("joint0bias", g_joint_bias[0]);
  //n_private.getParam("joint1bias", g_joint_bias[1]);
  ros::Publisher joint_pub = n.advertise<sensor_msgs::JointState>("master_state", 1);
  g_joint_pub = &joint_pub; // ugly ugly
  ros::Subscriber wiimote_sub = n.subscribe("wiimote/state", 1, wiimote_cb);
  ros::Subscriber wiimote2_sub = n.subscribe("wiimote2/state", 1, wiimote2_cb);
  tf::TransformBroadcaster tf_broadcaster;
  g_tf_broadcaster = &tf_broadcaster;
  ros::Rate loop_rate(100);
  geometry_msgs::TransformStamped world_trans;
  world_trans.header.frame_id = "world";
  world_trans.child_frame_id = "torso_link";
  world_trans.transform.translation.x = 0;
  world_trans.transform.translation.y = 0;
  world_trans.transform.translation.z = 0;
  world_trans.transform.rotation = tf::createQuaternionMsgFromRollPitchYaw(0,0,0);

  VectorXd init_mean(EKF::N);
  for (int i = 0; i < EKF::N; i++)
    init_mean[i] = 0; // at resting position, zero velocity

  MatrixXd init_cov = MatrixXd::Zero(EKF::N, EKF::N);
  for (int i = 0; i < EKF::NUM_JOINTS; i++)
    init_cov(i, i) = 0.5; // pretty sure we're nearby
  for (int i = EKF::NUM_JOINTS; i < EKF::N; i++)
    init_cov(i, i) = 0.1; // mostly stopped

  MatrixXd init_transition_cov = MatrixXd::Zero(EKF::N, EKF::N);
  for (int i = 0; i < EKF::NUM_JOINTS; i++)
    init_transition_cov(i, i) = 1e-4; // integrating states (mostly)
  for (int i = EKF::NUM_JOINTS; i < 2*EKF::NUM_JOINTS; i++)
    init_transition_cov(i, i) = 0.1; // allow room for angular acceleration

  MatrixXd init_meas_cov = MatrixXd::Zero(12, 12);
  for (int i = 0; i < 6; i++)
    init_meas_cov(i, i) = 0.5; // accelerometer quantization, noise
  for (int i = 6; i < 12; i++)
    init_meas_cov(i, i) = 0.2; // gyro noise

  EKF ekf(&tree);
  ekf.init(init_mean, init_cov, init_transition_cov, init_meas_cov);
  g_ekf = &ekf;

  while (ros::ok())
  {
    world_trans.header.stamp = ros::Time::now();
    tf_broadcaster.sendTransform(world_trans);
    loop_rate.sleep();
    ros::spinOnce();
  }
  return 0;
}

