#include "ros/ros.h"
#include "sensor_msgs/PointCloud.h"
#include "sensor_msgs/ChannelFloat32.h"
#include "geometry_msgs/Point32.h"
//#include "rosbag/bag.h"
#include <tf/transform_listener.h>
#include "recyclerbot/table_detector.h"
#include "recyclerbot/STANN/rand.hpp"

using namespace std;

class TableDetectorNode
{
public:
  ros::NodeHandle n;
	ros::Subscriber ntPoint_sub; // nt = narrow textured point cloud
	ros::Publisher filteredPoints_pub; // publish processed points
	ros::Publisher cylMarker_pub; // publish cylinders
	tf::TransformListener* tran; // narrow textured to Footprint
	ros::Time preStamp;
	TableDetector tableDetector;
  
  TableDetectorNode(ros::NodeHandle &_n) : n(_n)
  {
    filteredPoints_pub = n.advertise<sensor_msgs::PointCloud>("/filtered_points", 1);
    cylMarker_pub = n.advertise<visualization_msgs::Marker>("/bottle_marker", 3);
    ntPoint_sub = n.subscribe<sensor_msgs::PointCloud>("/narrow_stereo_textured/points", 1, &TableDetectorNode::nt_cb, this);
    tran = new tf::TransformListener(n, ros::Duration(2.0));
    preStamp = ros::Time::now();
//    ros::Timer timer = n.createTimer(ros::Duration(1.0), boost::bind(&TableDetectorNode::transformPoint, boost::ref(listener)));

  }
  
  ~TableDetectorNode()
  {
    ROS_INFO("TableDetectorNode destructor");
    delete tran;
  }
  
  void nt_cb(const sensor_msgs::PointCloud::ConstPtr &msg); // narrow textured call back
};

void TableDetectorNode::nt_cb(const sensor_msgs::PointCloud::ConstPtr &msg) // narrow textured call back
{
	// tranform point cloud into /base_footprint frame
	sensor_msgs::PointCloud transformedPoints;
	if (preStamp >= msg->header.stamp)
	{
		cout<<"flush listener"<<endl;
		preStamp = msg->header.stamp;
		tran->clear();
		return;
	}
	preStamp = msg->header.stamp;
	try 
	{
		tran->transformPointCloud("/base_footprint", *msg, transformedPoints);
  } 
  catch (tf::TransformException& ex) 
  {
  	return;
  }
  
	sensor_msgs::PointCloud filtered_msg;
	visualization_msgs::Marker marker_msg[3];
	
	// copy header info into new message
	filtered_msg.header.frame_id = "/base_footprint";//msg->header.frame_id;
	filtered_msg.header.stamp = ros::Time::now();
	filtered_msg.header.seq = msg->header.seq;
	
	int i = 0;
	for (i = 0; i < 3; i++)
	{
		marker_msg[i].header.frame_id = "/base_footprint";
		marker_msg[i].header.stamp = ros::Time::now();
		marker_msg[i].ns = "bottle";
		marker_msg[i].id = i;
		marker_msg[i].type = visualization_msgs::Marker::CYLINDER;
		marker_msg[i].action = visualization_msgs::Marker::ADD;
		marker_msg[i].color.r = (i == 0 ? 1.0f:0.0f);
		marker_msg[i].color.g = (i == 1 ? 1.0f:0.0f);
		marker_msg[i].color.b = (i == 2 ? 1.0f:0.0f);
		marker_msg[i].color.a = 0.5;
		marker_msg[i].lifetime = ros::Duration();
	}
	
	// get table point cloud
	tableDetector.process_cloud(transformedPoints, &filtered_msg, marker_msg);
	
	if (n.ok()) 
	{
		filteredPoints_pub.publish(filtered_msg);
		for (i = 0; i < 3; i++) cylMarker_pub.publish(marker_msg[i]);
		//cylMarker_pub.publish(marker_msg[0]);
		//cylMarker_pub.publish(marker_msg[2]);
		//cylMarker_pub.publish(marker_msg[1]);
	}

}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "table_detector");
  ros::NodeHandle n;
  __srand48__(static_cast<unsigned int>(time(0)));
  TableDetectorNode tdn(n);
	ros::spin();
  return 0;
}


