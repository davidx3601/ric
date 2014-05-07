

void setup_driver() {

  float pid_constatns[5];
  if (!nh.getParam("pid_constants", pid_constatns, 5)) {
    nh.logwarn("No PID parameters found, using defaults.");
  }
  else {
    nh.loginfo("PID parameters found, sending to controller...");
    cmdMessenger.sendCmdStart(kSetParameters);
    cmdMessenger.sendCmdArg(pid_constatns[0],5);
    cmdMessenger.sendCmdArg(pid_constatns[1],5);
    cmdMessenger.sendCmdArg(pid_constatns[2],5);
    cmdMessenger.sendCmdArg(pid_constatns[3],5);
    cmdMessenger.sendCmdArg((int)pid_constatns[4]);
    cmdMessenger.sendCmdEnd();

  }

  stop_motors();
  got_parameters=false;
  encoders_ok=false;
  gotp_t=millis();
  enc_ok_t=millis();

}

void OnGetParametersAck() {
  got_parameters=true;
  nh.loginfo("Got parameters from controller:");
  float kp = cmdMessenger.readFloatArg();
  float ki = cmdMessenger.readFloatArg();
  float kd = cmdMessenger.readFloatArg();
  float alpha = cmdMessenger.readFloatArg();
  int cdt = cmdMessenger.readIntArg();

  char log_msg[30];
  char log_msg1[10];

  dtostrf(kp,3,4,log_msg1);
  sprintf(log_msg, "    kp = %s", log_msg1);
  nh.loginfo(log_msg);
  
 dtostrf(ki,3,4,log_msg1);
  sprintf(log_msg, "    ki = %s", log_msg1);
  nh.loginfo(log_msg);

 dtostrf(kd,3,4,log_msg1);
  sprintf(log_msg, "    kd = %s", log_msg1);
  nh.loginfo(log_msg);

dtostrf(alpha,3,4,log_msg1);
  sprintf(log_msg, "    alpha = %s", log_msg1);
  nh.loginfo(log_msg);

  sprintf(log_msg, "    Control loop dt = %d", cdt);
  nh.loginfo(log_msg);
  nh.loginfo("Controller ready");
}


void OnEncoders() {
  left_enc = cmdMessenger.readIntArg();
  right_enc = cmdMessenger.readIntArg();
  enc_ok_t=millis();
  if (!encoders_ok){
      nh.loginfo("Communication with controller is OK");
      encoders_ok=true;
    }
}

void check_encoders() {
  if (millis()-enc_ok_t>2000) {
    if (encoders_ok){
      nh.logwarn("No communication with controller");
      encoders_ok=false;
    }
  } 
}

void OnStatus() {
  RxStatus = cmdMessenger.readBoolArg();
  controller_bat_v = cmdMessenger.readFloatArg();
}


void read_status() {

int gps_fault_bit=0;
#ifdef USE_GPS
 // gps_fault_bit = !gps.location.isValid();
 if (gps.location.age()>GPS_IS_OLD) gps_fault_bit=1;
#endif

  status_msg.faults = 8 * (int)imu_fault + 4 * gps_fault_bit + 2*(int)(!encoders_ok) + 1*(int)(RxStatus);
  status_msg.battery1_voltage = (float)analogRead(BATTERY_MONITOR_PIN) * 3.3 / 65535 * VOLTAGE_DIVIDER_RATIO;
  status_msg.battery2_voltage = controller_bat_v;
  p_status.publish(&status_msg);

}



void reset_encCb(const Empty::Request & req, Empty::Response & res) {
  cmdMessenger.sendCmd (kReset, true);

  nh.loginfo("Reset encoders");


}

void commandCb( const ric_robot::ric_command& msg) {

  cmdMessenger.sendCmdStart(kCommand);
  cmdMessenger.sendCmdArg(msg.left_wheel);
  cmdMessenger.sendCmdArg(msg.right_wheel);
  cmdMessenger.sendCmdEnd();

}

void stop_motors() {

  cmdMessenger.sendCmdStart(kCommand);
  cmdMessenger.sendCmdArg(0.0);
  cmdMessenger.sendCmdArg(0.0);
  cmdMessenger.sendCmdEnd();
}




