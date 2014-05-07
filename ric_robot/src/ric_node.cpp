#include "ros/ros.h"
#include <ric_robot/ric_raw.h>
#include <ric_robot/ric_gps.h>
#include <ric_robot/ric_pan_tilt.h>
#include <ric_robot/ric_command.h>
#include <ric_robot/set_odom.h>
#include "sensor_msgs/Imu.h"
#include "sensor_msgs/Range.h"
#include "sensor_msgs/NavSatFix.h"
#include "sensor_msgs/NavSatStatus.h"
#include <sensor_msgs/JointState.h>
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/Quaternion.h"
#include "geometry_msgs/Vector3Stamped.h"
#include "nav_msgs/Odometry.h"
#include "tf/transform_datatypes.h"
#include <tf/transform_broadcaster.h>
#include <math.h>


ros::Publisher left_urf_pub;
ros::Publisher rear_urf_pub;
ros::Publisher right_urf_pub;

ros::Publisher odom_pub;
ros::Publisher gps_pub;
ros::Publisher imu_pub;
ros::Publisher command_pub;
ros::Publisher pan_tilt_pub;


sensor_msgs::Range left_urf_msg;
sensor_msgs::Range rear_urf_msg;
sensor_msgs::Range right_urf_msg;

//std::string ric_id;
std::string gps_frame_id;
std::string odom_frame_id;
std::string base_frame_id;
std::string imu_frame_id;
std::string cmd_vel_topic;
std::string pan_tilt_topic;
std::string ric_command_topic;
std::string gps_pub_topic;
std::string set_odom_srv;
std::string odom_topic;
std::string ric_raw_topic;
std::string ric_gps_topic;
std::string left_urf_frame_id;
std::string rear_urf_frame_id;
std::string right_urf_frame_id;
std::string pan_joint_id;
std::string tilt_joint_id;
std::string FR_joint_id;
std::string FL_joint_id;
std::string RR_joint_id;
std::string RL_joint_id;

std::string joint_states_topic;

sensor_msgs::JointState jointstate_msg;

tf::TransformBroadcaster *imu_broadcaster;
tf::TransformBroadcaster *odom_broadcaster;

double wheel_base_length;
double wheel_diameter;
double encoder_cpr; //=ppr*4

int l_encoder=0,pre_l_encoder=0;
int r_encoder=0,pre_r_encoder=0;
geometry_msgs::Quaternion q_imu;
double xx = 0, yy = 0, tt = 0;
bool first_enc_read = true;

bool fuse_imu_roll_pitch;
bool fuse_imu_yaw;

double rot_cov = 0.0;
double pos_cov = 0.0;
ros::Time prev_time;

bool wd_on=false;
ros::Time wd_time;

double pan=0;
double tilt=0;



void cmd_velCallback(const geometry_msgs::Twist::ConstPtr& msg) {
	
	if (wd_on) wd_on=false;
	
	wd_time = ros::Time::now();
	
	
	double Right_Motor_v = msg->linear.x + msg->angular.z * (wheel_base_length / 2.0);
	double Left_Motor_v =  msg->linear.x - msg->angular.z * (wheel_base_length / 2.0);

	ric_robot::ric_command command_msg;
	command_msg.left_wheel=(int)(Left_Motor_v*encoder_cpr/(M_PI * wheel_diameter)); //tick per sec
	command_msg.right_wheel=(int)(Right_Motor_v*encoder_cpr/(M_PI * wheel_diameter)); //tick per sec
	command_pub.publish(command_msg);



}

double wrapToPi(double angle) {
	angle += M_PI;
	bool is_neg = (angle < 0);
	angle = fmod(angle, (2.0 * M_PI));
	if (is_neg) {
		angle += (2.0 * M_PI);
	}
	angle -= M_PI;
	return angle;
}

void encodersCallback(int32_t left_ticks, int32_t right_ticks) {
	
	ros::Time now = ros::Time::now();
		double delta_time = (now - prev_time).toSec();
		prev_time = now;
	
		pre_l_encoder=l_encoder;
		pre_r_encoder=r_encoder;
				
		l_encoder = left_ticks;
		r_encoder = right_ticks;
	

		////velocity from driver encoder readings
		double l_tics_per_s=(double)(l_encoder-pre_l_encoder)/delta_time;
		double r_tics_per_s=(double)(r_encoder-pre_r_encoder)/delta_time;
		
		double l_w = l_tics_per_s * 2.0 * M_PI / (double) encoder_cpr;
			double r_w = r_tics_per_s * 2.0 * M_PI / (double) encoder_cpr;
			
		
		
		double l_v = l_w * wheel_diameter / 2.0;
		double r_v = r_w * wheel_diameter / 2.0;

		double v = (l_v + r_v) / 2.0;

		double w = (r_v - l_v) / wheel_base_length;

		if (first_enc_read) {

				xx = 0;
				yy = 0;
				tt = 0;
				first_enc_read = false;
			}
		
		tt = tt + w * delta_time;
		tt = wrapToPi(tt);
		xx = xx + (v * cos(tt))*delta_time;
		yy = yy + (v * sin(tt))*delta_time;

		
		
		nav_msgs::Odometry odom_msg;

		geometry_msgs::Quaternion q_odom;
		
		odom_msg.header.stamp = now;
		odom_msg.header.frame_id = odom_frame_id;
		odom_msg.pose.pose.position.x = xx;
		odom_msg.pose.pose.position.y = yy;
		odom_msg.pose.pose.position.z = 0;
		
		if ((fuse_imu_roll_pitch)&&(fuse_imu_yaw)) {
			odom_msg.pose.pose.orientation = q_odom=q_imu;		
		}
		else if ((!fuse_imu_roll_pitch)&&(fuse_imu_yaw)) {
			double roll = 0, pitch = 0, yaw = 0;					
			tf::Quaternion q;		
			tf::quaternionMsgToTF(q_imu, q);				
			tf::Matrix3x3(q).getRPY(roll, pitch, yaw);	
			q_odom =tf::createQuaternionMsgFromRollPitchYaw(0,0, yaw);
		}
		else if ((fuse_imu_roll_pitch)&&(!fuse_imu_yaw)) {
					double roll = 0, pitch = 0, yaw = 0;					
					tf::Quaternion q;		
					tf::quaternionMsgToTF(q_imu, q);				
					tf::Matrix3x3(q).getRPY(roll, pitch, yaw);	
					q_odom =tf::createQuaternionMsgFromRollPitchYaw(roll,pitch, tt);
				}	
		else if ((!fuse_imu_roll_pitch)&&(!fuse_imu_yaw)) {
			q_odom =tf::createQuaternionMsgFromRollPitchYaw(0,0, tt);
		}
	
		odom_msg.pose.pose.orientation=q_odom;
		
		odom_msg.child_frame_id = base_frame_id;
		odom_msg.twist.twist.linear.x = v;
		odom_msg.twist.twist.linear.y = 0;
		odom_msg.twist.twist.angular.z = w;

		odom_msg.pose.covariance[0] = pos_cov;
		odom_msg.pose.covariance[7] = pos_cov;
		odom_msg.pose.covariance[14] = 1e100;
		odom_msg.pose.covariance[21] = 1e100;
		odom_msg.pose.covariance[28] = 1e100;
		odom_msg.pose.covariance[35] = rot_cov;
		odom_msg.twist.covariance = odom_msg.pose.covariance;

		odom_pub.publish(odom_msg);

		//Add TF broadcaster
		geometry_msgs::TransformStamped odom_trans;
		odom_trans.header.stamp = now;
		odom_trans.header.frame_id = odom_frame_id;
		odom_trans.child_frame_id = base_frame_id;
		odom_trans.transform.translation.x = xx;
		odom_trans.transform.translation.y = yy;
		odom_trans.transform.translation.z = 0.0;
	odom_trans.transform.rotation = q_odom;
	
			
		odom_broadcaster->sendTransform(odom_trans);


	jointstate_msg.header.stamp = ros::Time::now();
	jointstate_msg.position[0]=pan;
	jointstate_msg.position[1]=tilt;

	//TODO: Publish wheels joints

	pan_tilt_pub.publish(jointstate_msg);

}

void gpsCallback(const ric_robot::ric_gps::ConstPtr& msg) {

	
	sensor_msgs::NavSatFix gps_msg;
	gps_msg.header.frame_id = gps_frame_id;
	gps_msg.header.stamp = ros::Time::now();
	gps_msg.latitude = msg->Lat;
	gps_msg.longitude = msg->Lon;
	gps_msg.altitude = msg->Alt;

	sensor_msgs::NavSatStatus gps_status_msg;

	gps_status_msg.service = 1; //SERVICE_GPS;
	if (msg->Status == 0) {
		gps_status_msg.status = 1; //STATUS_NO_FIX;
	}
	else {//==1
		gps_status_msg.status = 0; //STATUS_FIX;
	//ROS_INFO("lat:%.7f  lon:%.7f",msg->Lat,msg->Lon  );
	}

	gps_msg.status = gps_status_msg;

	gps_msg.position_covariance_type = 1;
	//uint8 COVARIANCE_TYPE_UNKNOWN = 0
	//uint8 COVARIANCE_TYPE_APPROXIMATED = 1
	//uint8 COVARIANCE_TYPE_DIAGONAL_KNOWN = 2
	//uint8 COVARIANCE_TYPE_KNOWN = 3
	//float64[9] position_covariance;
	
	float cov =  (msg->HDOP) * (msg->HDOP);  //TODO: CHECK UNITS /100
	gps_msg.position_covariance[0] = cov;
	gps_msg.position_covariance[4] = cov;
	gps_msg.position_covariance[8] = cov;

	gps_pub.publish(gps_msg);
	//gps status:
	//NO_GPS = 0,             ///< No GPS connected/detected
	//  NO_FIX = 1,             ///< Receiving valid GPS messages but no lock
	//  GPS_OK = 2              ///< Receiving valid messages and locked
}

void urfCallback(float left_urf,float rear_urf,float right_urf) {
	
	
	ros::Time now = ros::Time::now();

	left_urf_msg.header.stamp = now;
	left_urf_msg.range = left_urf ;

	rear_urf_msg.header.stamp = now;
	rear_urf_msg.range = rear_urf ;
	
	right_urf_msg.header.stamp = now;
	right_urf_msg.range = right_urf ;

	
	left_urf_pub.publish(left_urf_msg);
	rear_urf_pub.publish(rear_urf_msg);
	right_urf_pub.publish(right_urf_msg);
	
}



void imuCallback(float qw,float qx,float qy,float qz) {


	sensor_msgs::Imu imu_msg;
	
	imu_msg.header.frame_id = imu_frame_id;
	imu_msg.header.stamp = ros::Time::now();
	q_imu.w=qw;
	q_imu.x=qx;
	q_imu.y=qy;
	q_imu.z=qz;
	
	double roll = 0, pitch = 0, yaw = 0;							
	tf::Quaternion q;			
	tf::quaternionMsgToTF(q_imu, q);							
	tf::Matrix3x3(q).getRPY(roll, pitch, yaw);
	
	q_imu =tf::createQuaternionMsgFromRollPitchYaw(roll,-pitch, -yaw);

	imu_msg.orientation=q_imu;
		

	double angular_velocity_stdev_ = 0.012;
	double orientation_stdev_ = 0.035;
	double linear_acceleration_stdev_ = 0.098;
	
	double angular_velocity_covariance = angular_velocity_stdev_
			* angular_velocity_stdev_;
	double orientation_covariance = orientation_stdev_ * orientation_stdev_;
	double linear_acceleration_covariance = linear_acceleration_stdev_
			* linear_acceleration_stdev_;

	imu_msg.linear_acceleration_covariance[0] = linear_acceleration_covariance;
	imu_msg.linear_acceleration_covariance[4] = linear_acceleration_covariance;
	imu_msg.linear_acceleration_covariance[8] = linear_acceleration_covariance;

	imu_msg.angular_velocity_covariance[0] = angular_velocity_covariance;
	imu_msg.angular_velocity_covariance[4] = angular_velocity_covariance;
	imu_msg.angular_velocity_covariance[8] = angular_velocity_covariance;

	imu_msg.orientation_covariance[0] = orientation_covariance;
	imu_msg.orientation_covariance[4] = orientation_covariance;
	imu_msg.orientation_covariance[8] = orientation_covariance;
	imu_pub.publish(imu_msg);
	
	/*
	geometry_msgs::TransformStamped imu_trans;
	 imu_trans.header.stamp = ros::Time::now();
	 imu_trans.header.frame_id = odom_frame_id;
	 imu_trans.child_frame_id = imu_frame_id;
	 imu_trans.transform.rotation=q_imu;

	 imu_broadcaster->sendTransform(imu_trans);
*/


}
void rawCallback(const ric_robot::ric_raw::ConstPtr& msg) {
float qw=msg->qw;
float qx=msg->qx;
float qy=msg->qy;
float qz=msg->qz;
imuCallback(qw,qx,qy,qz);

int32_t left_ticks=msg->left_ticks;
int32_t right_ticks=msg->right_ticks;
encodersCallback( left_ticks,  right_ticks);

float left_urf=msg->left_urf;
float rear_urf=msg->rear_urf;
float right_urf=msg->right_urf;
urfCallback(left_urf, rear_urf, right_urf);

}

bool set_odom(ric_robot::set_odom::Request  &req, ric_robot::set_odom::Response &res)
{

	xx=req.x;
	yy=req.y;
	tt=req.theta;
 ROS_INFO("Set odometry to: x=%.3f, y=%.3f, theta=%.3f", xx, yy,tt);
 
 return true;
}


void pan_tiltCallback(const ric_robot::ric_pan_tilt::ConstPtr& msg) {

	
	
	pan=-1.0*msg->pan_angle;
	tilt=-1.0*msg->tilt_angle;
	if (pan*180/M_PI>35) pan=35*M_PI/180;
	else if (pan*180/M_PI<-35) pan=-35*M_PI/180;

	if (tilt*180/M_PI>30) tilt=30*M_PI/180;
	else if (tilt*180/M_PI<-30) tilt=-30*M_PI/180;


}
int main(int argc, char **argv) {


	ros::init(argc, argv, "ric_node", ros::init_options::AnonymousName);

	ros::NodeHandle n;
	int ricID;

	n.param("GPS_frame_id", gps_frame_id, std::string("/map"));

	std::string pre = ros::this_node::getNamespace();
	std::string pre1 = pre.substr (2,pre.length()-2); 

	n.param("odom_frame_id", odom_frame_id, std::string(pre1+"_odom"));
	n.param("base_frame_id", base_frame_id, std::string(pre1+"_base_link"));
	n.param("imu_frame_id", imu_frame_id, std::string(pre1+"_imu_link"));
	n.param("left_urf_frame_id", left_urf_frame_id, std::string(pre1+"_Left_URF_link"));
	n.param("rear_urf_frame_id", rear_urf_frame_id, std::string(pre1+"_Rear_URF_link"));
	n.param("right_urf_frame_id", right_urf_frame_id, std::string(pre1+"_Right_URF_link"));
	n.param("pan_joint_id", pan_joint_id, std::string(pre1+"_Asus_Pan_Joint"));
	n.param("tilt_joint_id", tilt_joint_id, std::string(pre1+"_Asus_Tilt_Joint"));
	n.param("FR_joint_id", FR_joint_id, std::string(pre1+"_FR_Wheel_Joint"));
	n.param("FL_joint_id", FL_joint_id, std::string(pre1+"_FL_Wheel_Joint"));
	n.param("RR_joint_id", RR_joint_id, std::string(pre1+"_RR_Wheel_Joint"));
	n.param("RL_joint_id", RL_joint_id, std::string(pre1+"_RL_Wheel_Joint"));

	n.param("cmd_vel_sub_topic", cmd_vel_topic, std::string("cmd_vel"));
	n.param("pan_tilt_sub_topic", pan_tilt_topic, std::string("pan_tilt"));
	
	n.param("ric_raw_sub_topic", ric_raw_topic, std::string("ric_raw"));
	n.param("ric_gps_sub_topic", ric_gps_topic, std::string("raw_gps"));

	n.param("odom_pub_topic", odom_topic, std::string("odom_pub"));
	n.param("gps_pub_topic", gps_pub_topic, std::string("gps_pub"));	

	n.param("joint_states_topic", joint_states_topic, std::string("joint_states"));	

	n.param("ric_command_pub_topic", ric_command_topic, std::string("command"));

	n.param("set_odom_srv", set_odom_srv, std::string("set_odom"));

	n.param("wheel_diameter", wheel_diameter, 0.12);
	n.param("wheel_base_length", wheel_base_length, 0.275);
	n.param("encoder_cpr", encoder_cpr, 4288.0);
	n.param("rotation_covariance", rot_cov, 1.0);
	n.param("position_covariance", pos_cov, 1.0);
	n.param("fuse_imu_roll_pitch", fuse_imu_roll_pitch, true);
	n.param("fuse_imu_yaw", fuse_imu_yaw, false);
		
	q_imu.x=0;
	q_imu.y=0;
	q_imu.z=1;
	q_imu.w=0;


	ros::ServiceServer service = n.advertiseService(set_odom_srv, set_odom);
		
	
	ros::Subscriber raw_sub = n.subscribe(ric_raw_topic, 10, rawCallback);
	ros::Subscriber gps_sub = n.subscribe(ric_gps_topic, 10, gpsCallback);
	ros::Subscriber vel_sub = n.subscribe(cmd_vel_topic, 5, cmd_velCallback);

	ros::Subscriber pan_tilt_sub = n.subscribe(pan_tilt_topic, 1, pan_tiltCallback);

	imu_pub = n.advertise < sensor_msgs::Imu > ("imu_pub", 10);

	
	jointstate_msg.name.push_back(pan_joint_id);
	jointstate_msg.position.push_back(0);
	jointstate_msg.name.push_back(tilt_joint_id);
	jointstate_msg.position.push_back(0);

	jointstate_msg.name.push_back(FR_joint_id);
	jointstate_msg.position.push_back(0);
	jointstate_msg.name.push_back(FL_joint_id);
	jointstate_msg.position.push_back(0);
	jointstate_msg.name.push_back(RR_joint_id);
	jointstate_msg.position.push_back(0);
	jointstate_msg.name.push_back(RL_joint_id);
	jointstate_msg.position.push_back(0);

	right_urf_msg.header.frame_id = right_urf_frame_id;
	right_urf_msg.field_of_view = 0.7;
	right_urf_msg.radiation_type = 0; //us
	right_urf_msg.min_range = 0.25;
	right_urf_msg.max_range = 6;

	rear_urf_msg.header.frame_id = rear_urf_frame_id;
	rear_urf_msg.field_of_view = 0.7;
	rear_urf_msg.radiation_type = 0; //us
	rear_urf_msg.min_range = 0.25;
	rear_urf_msg.max_range = 6;

	left_urf_msg.header.frame_id = left_urf_frame_id;
	left_urf_msg.field_of_view = 0.7;
	left_urf_msg.radiation_type = 0; //us
	left_urf_msg.min_range = 0.25;
	left_urf_msg.max_range = 6;


	right_urf_pub = n.advertise < sensor_msgs::Range > ("Rangers/Right_URF", 5);
	rear_urf_pub = n.advertise < sensor_msgs::Range > ("Rangers/Rear_URF", 5);
	left_urf_pub = n.advertise < sensor_msgs::Range > ("Rangers/Left_URF", 5);
	
	odom_pub = n.advertise < nav_msgs::Odometry > (odom_topic, 10);


	command_pub = n.advertise < ric_robot::ric_command > (ric_command_topic, 5);
	pan_tilt_pub = n.advertise < sensor_msgs::JointState > (joint_states_topic, 5); //

	//pan_tilt_pub.publish(jointstate_msg);

	gps_pub = n.advertise < sensor_msgs::NavSatFix > (gps_pub_topic, 5);


	imu_broadcaster = new tf::TransformBroadcaster;

	odom_broadcaster = new tf::TransformBroadcaster;
	wd_time=ros::Time::now();
	while (n.ok()) {

/*	 if (((ros::Time::now() - wd_time).toSec()>3)&&(!wd_on)) {  	    
		 wd_on=true;
		 ric_robot::ric_command command_msg;	 	
		 command_msg.left_wheel=0;
		 command_msg.right_wheel=0;	 	
		 command_pub.publish(command_msg);
	 }*/
	 ros::spinOnce();
	}
		
	//ros::spin();

	return 0;
}

