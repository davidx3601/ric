#!/usr/bin/env python
import roslib; roslib.load_manifest('ric_robot')
import rospy
import math
from ric_robot.msg import *
from std_msgs.msg import Float64
from math import *
from sensor_msgs.msg import JointState

def rc_callback(data):
  global right_finger_pub,right_finger_min,right_finger_max
  global left_finger_pub,left_finger_min,left_finger_max
  global wrist_pub,wrist_min,wrist_max, wrist_ang
  global elbow2_pub,elbow2_min,elbow2_max, elbow2_ang
  global elbow1_pub,elbow1_min,elbow1_max, elbow1_ang
  global shoulder_pub,shoulder_min,shoulder_max, shoulder_ang
  global base_rotation_pub,base_rotation_min,base_rotation_max, base_rotation_ang
  #rospy.loginfo(data.RX1)
  msg=right_finger_min+((right_finger_max-right_finger_min)/(2000.0-1000.0))*(data.RX3-1000.0)
  msg=math.pi-msg*2.0*math.pi/4096.0
  right_finger_pub.publish(-msg)
  
  msg=left_finger_min+((left_finger_max-left_finger_min)/(2000.0-1000.0))*(data.RX3-1000.0)
  msg=math.pi-msg*2.0*math.pi/4096.0
  left_finger_pub.publish(msg)
  
  if data.RX4 > 1600:
     wrist_ang+=0.03
  elif data.RX4 < 1400:
     wrist_ang-=0.03
  if wrist_ang> math.pi-wrist_min*2.0*math.pi/4096.0:
    wrist_ang=math.pi-wrist_min*2.0*math.pi/4096.0
  elif wrist_ang< math.pi-wrist_max*2.0*math.pi/4096.0:
    wrist_ang=math.pi-wrist_max*2.0*math.pi/4096.0
  wrist_pub.publish(wrist_ang)
  
  if data.RX6 < 1500:
     if data.RX2 > 1600:
        elbow2_ang+=0.03
     elif data.RX2 < 1400:
        elbow2_ang-=0.03
     if elbow2_ang> math.pi-elbow2_min*2.0*math.pi/4096.0:
       elbow2_ang=math.pi-elbow2_min*2.0*math.pi/4096.0
     elif elbow2_ang< math.pi-elbow2_max*2.0*math.pi/4096.0:
       elbow2_ang=math.pi-elbow2_max*2.0*math.pi/4096.0
     elbow2_pub.publish(elbow2_ang) 
  
     if data.RX1 > 1600:
        elbow1_ang-=0.03
     elif data.RX1 < 1400:
        elbow1_ang+=0.03
     if elbow1_ang> math.pi-elbow1_min*2.0*math.pi/4096.0:
       elbow1_ang=math.pi-elbow1_min*2.0*math.pi/4096.0
     elif elbow1_ang< math.pi-elbow1_max*2.0*math.pi/4096.0:
       elbow1_ang=math.pi-elbow1_max*2.0*math.pi/4096.0
     elbow1_pub.publish(elbow1_ang) 
  else:
     if data.RX2 > 1600:
        shoulder_ang+=0.03
     elif data.RX2 < 1400:
        shoulder_ang-=0.03
     if shoulder_ang> math.pi-shoulder_min*2.0*math.pi/4096.0:
       shoulder_ang=math.pi-shoulder_min*2.0*math.pi/4096.0
     elif shoulder_ang< math.pi-shoulder_max*2.0*math.pi/4096.0:
       shoulder_ang=math.pi-shoulder_max*2.0*math.pi/4096.0
     shoulder_pub.publish(shoulder_ang) 
  
     if data.RX1 > 1600:
        base_rotation_ang-=0.03
     elif data.RX1 < 1400:
        base_rotation_ang+=0.03
     if base_rotation_ang> math.pi-base_rotation_min*2.0*math.pi/4096.0:
       base_rotation_ang=math.pi-base_rotation_min*2.0*math.pi/4096.0
     elif base_rotation_ang< math.pi-base_rotation_max*2.0*math.pi/4096.0:
       base_rotation_ang=math.pi-base_rotation_max*2.0*math.pi/4096.0
     base_rotation_pub.publish(base_rotation_ang) 
     
  #rospy.loginfo(shoulder_ang)
  #rospy.loginfo(math.pi-wrist_max*2.0*math.pi/4096.0)
  #rospy.loginfo(math.pi-wrist_min*2.0*math.pi/4096.0)
  #rospy.loginfo(data.RX6)
  #rospy.loginfo("--")
def ric_arm_rc():
   global right_finger_pub,right_finger_min,right_finger_max
   global left_finger_pub,left_finger_min,left_finger_max
   global wrist_pub,wrist_min,wrist_max, wrist_ang
   global elbow2_pub,elbow2_min,elbow2_max, elbow2_ang
   global elbow1_pub,elbow1_min,elbow1_max, elbow1_ang
   global shoulder_pub,shoulder_min,shoulder_max, shoulder_ang
   global base_rotation_pub,base_rotation_min,base_rotation_max, base_rotation_ang
   wrist_ang=0.0;
   elbow2_ang=0.0;
   elbow1_ang=0.0;
   shoulder_ang=0.0;
   base_rotation_ang=0.0;
   ns=rospy.get_namespace()
   rospy.init_node('ric_gui', anonymous=True)
   ns=rospy.get_namespace()
   right_finger_max = rospy.get_param("right_finger_controller/motor/max")
   right_finger_min = rospy.get_param("right_finger_controller/motor/min")
   left_finger_max = rospy.get_param("left_finger_controller/motor/max")
   left_finger_min = rospy.get_param("left_finger_controller/motor/min")
   wrist_max = rospy.get_param("wrist_controller/motor/max")
   wrist_min = rospy.get_param("wrist_controller/motor/min")
   elbow2_max = rospy.get_param("elbow2_controller/motor/max")
   elbow2_min = rospy.get_param("elbow2_controller/motor/min")
   elbow1_max = rospy.get_param("elbow1_controller/motor/max")
   elbow1_min = rospy.get_param("elbow1_controller/motor/min")
   shoulder_max = rospy.get_param("shoulder_controller/motor/max")
   shoulder_min = rospy.get_param("shoulder_controller/motor/min")
   base_rotation_max = rospy.get_param("base_rotation_controller/motor/max")
   base_rotation_min = rospy.get_param("base_rotation_controller/motor/min")
   rospy.Subscriber(ns+"RC",ric_rc, rc_callback)
   left_finger_pub = rospy.Publisher(ns+"left_finger_controller/command", Float64);
   right_finger_pub = rospy.Publisher(ns+"right_finger_controller/command", Float64)
   wrist_pub = rospy.Publisher(ns+"wrist_controller/command", Float64)
   elbow2_pub = rospy.Publisher(ns+"elbow2_controller/command", Float64)   
   elbow1_pub = rospy.Publisher(ns+"elbow1_controller/command", Float64)   
   shoulder_pub = rospy.Publisher(ns+"shoulder_controller/command", Float64)   
   base_rotation_pub = rospy.Publisher(ns+"base_rotation_controller/command", Float64) 
  
   while not rospy.is_shutdown():
     #rospy.loginfo("test")
     rospy.sleep(0.1)
        
if __name__ == '__main__':
   ric_arm_rc()
	