class alldir: public Move_Robot
{
	public:

		alldir(char *dev_name, int Baudrate);
		~alldir();

		ros::Subscriber Clear_ObsModeSubscriber_;



		//Timer & Thread & SERIAL
		//void timerCallback(const ros::TimerEvent& event);
		void RevProcess(double receive_period);
		void RevProcess_Battery(double receive_period);
		void timerCallback(const ros::TimerEvent& event);
		void ClearCallback(const std_msgs::Int8& msg);
		void laserCallback(const sensor_msgs::LaserScan& scan);
		void joystickCallback(const move_robot::joystick& joystick);
		void TriggerCallback_all(const std_msgs::Int8& msg);



		//state
		void State_Machine(int& state);
		void Idle();
		void Stop();

		//tracking
		bool Navigation_move(std::vector<SUB_MISSONPATH_SUBPOINT> & myAllSubPath, bool isReSet);
		bool Tracking_Angle_Init(int &subpath_index, bool isReSet);
		bool Tracking_Trajectory(int &subpath_index, bool isReSet);
		bool Trajectory_Tracking(int &subpath_index, bool isReSet);

		//OBS
		void ObsDetect(std::vector<Eigen::Vector3f> &i_obs_container,
				std::vector<Eigen::Vector3f> &i_obs_region_0,
				std::vector<Eigen::Vector3f> &i_obs_region_1,
				std::vector<Eigen::Vector3f> &i_obs_region_2,
				std::vector<Eigen::Vector3f> &i_obs_region_3,
				std::vector<Eigen::Vector3f> &i_obs_region_4,
				std::vector<Eigen::Vector3f> &i_obs_gap_0,
				std::vector<Eigen::Vector3f> &i_obs_gap_2,
				std::vector<Eigen::Vector3f> &i_obs_gap_4,
				std::vector<Eigen::Vector3f> &all_point);


		void ObsStateMachine(std::vector<Eigen::Vector3f> &i_obs_region_0,
				std::vector<Eigen::Vector3f> &i_obs_region_1,
				std::vector<Eigen::Vector3f> &i_obs_region_2,
				std::vector<Eigen::Vector3f> &i_obs_region_3,
				std::vector<Eigen::Vector3f> &i_obs_region_4,
				std::vector<Eigen::Vector3f> &i_obs_gap_0,
				std::vector<Eigen::Vector3f> &i_obs_gap_2,
				std::vector<Eigen::Vector3f> &i_obs_gap_4,
				std::vector<Eigen::Vector3f> &all_point);


		void RePlanning(int &subpath_index,
				std::vector<Eigen::Vector3f> &i_obs_gap_0,
				std::vector<Eigen::Vector3f> &i_obs_gap_2,
				std::vector<Eigen::Vector3f> &i_obs_gap_4,
				std::vector<Eigen::Vector3f> &all_point);

		bool FindKeyPoint(Eigen::Vector3f& target, Eigen::Vector3f& key_p,
				std::vector<Eigen::Vector3f> &i_obs_gap_0,
				std::vector<Eigen::Vector3f> &i_obs_gap_2,
				std::vector<Eigen::Vector3f> &i_obs_gap_4,
				std::vector<Eigen::Vector3f> &all_point);

		void CreateNewPath(Eigen::Vector3f& target, Eigen::Vector3f& key_point, int subpath_index, std::vector<Eigen::Vector3f> &new_path);
		bool WaitObsLeave();
		bool WaitDeadlock();
		bool AvoidObs();

		//joystick
		void joystick_move();
		void joystick_omni_move();

		//里程計 使用
		void Calculate_odom();

		//切換diff或omni模式 V跟W
		void Caculate_W_rw(float stop_angle, Eigen::Vector3f robot_pos, float &angular_error, float &pre_angular_error, float &cmd_angular_velocity, bool special);



	private:
		//encoder
		float Rev_FR_rpm, Rev_FL_rpm, Rev_RR_rpm, Rev_RL_rpm;
		float Rev_FR_deg, Rev_FL_deg, Rev_RR_deg, Rev_RL_deg;

		//里程計 使用
		float Rev_odom_V1, Rev_odom_V2, Rev_odom_V3, Rev_odom_V4;
		float Rev_odom_t1, Rev_odom_t2, Rev_odom_t3, Rev_odom_t4;
		float Rev_odom_vx, Rev_odom_vy, Rev_odom_v, Rev_odom_w;

		float ini_way_theta;

};

alldir::alldir(char* dev_name, int Baudrate):Move_Robot(dev_name, Baudrate)
{



	//revprocess
	Rev_FR_rpm = 0;
	Rev_FL_rpm = 0;
	Rev_RR_rpm = 0;
	Rev_RL_rpm = 0;

	Rev_FR_deg = 0;
	Rev_FL_deg = 0;
	Rev_RR_deg = 0;
	Rev_RL_deg = 0;

	//encoder 里程計 使用
	Rev_odom_V1 = 0;
	Rev_odom_V2 = 0;
	Rev_odom_V3 = 0;
	Rev_odom_V4 = 0;

	Rev_odom_t1 = 0;
	Rev_odom_t2 = 0;
	Rev_odom_t3 = 0;
	Rev_odom_t4 = 0;

	Rev_odom_vx = 0;
	Rev_odom_vy = 0;
	Rev_odom_v = 0;
	Rev_odom_w = 0;

	ini_way_theta = 0.0;

	std::cout<<"v_buf"<< v_buf<<std::endl;
	std::cout<<"alldir"<<std::endl;
	Clear_ObsModeSubscriber_=node_.subscribe("Clear_ObsMode", 10, &alldir::ClearCallback,this);
	laserSubscriber_ = node_.subscribe("scan", 10, &alldir::laserCallback, this);
	JoysickSubscriber_ = node_.subscribe("joystick", 5, &alldir::joystickCallback, this);
	TriggerSubscriber_=node_.subscribe("Trigger", 5, &alldir::TriggerCallback_all, this);



	receive_Battery_thread_ = new boost::thread(boost::bind(&alldir::RevProcess_Battery, this, 0.1));
	receive_thread_ = new boost::thread(boost::bind(&alldir::RevProcess, this, 0.01));
	control_timer = node_.createTimer(ros::Duration(time_sample), &alldir::timerCallback, this);


}
alldir::~alldir()
{

}
void alldir::TriggerCallback_all(const std_msgs::Int8& msg)
{

		int Trigger = msg.data;
		std::string LocalparPATH ;
		switch(Trigger)
		{
				case ReloadCarParameter:

							delete this;
							break;
				case ReloadLocalParameter:
							std::cout<<"ReloadLocalParameter"<<std::endl;
							LocalparPATH = TitlePath + LocalparPATH_Local;
						  sendreceive.Setmode(SetLocalpar(LocalparPATH));
							break;
				default:
							break;
		}
}
void alldir::timerCallback(const ros::TimerEvent& event)
{
	static bool LaserMiss_send = false;
	Calculate_odom();

	std_msgs::Float32 msg;
	msg.data = Rev_odom_v;
	v_Publisher_.publish(msg);
	if(isReveice_joystick){
		v_buf = 0.04;

		switch(btn_id){
			case PUSE_BUTTON_A:
				joystick_move();
				break;
			case PUSE_BUTTON_B:
				joystick_omni_move();
				break;
			case PUSE_BUTTON_X:
				joystick_move();
				break;

			case PUSE_BUTTON_Y:
				joystick_move();
				break;
			case PUSE_BUTTON_BACK:
				joystick_move();
				break;

			default:
				break;


		}

		isReveice_joystick = false;
		 LaserMiss_send = false;
	}
	else{


		LaserMiss += 1;

		if(LaserMiss > 5)
		{
			Stop();
			std_msgs::Int8 msg;
			msg.data = 6;
			if(!LaserMiss_send)
			{
				Error_Publisher_.publish(msg);
				LaserMiss_send = true;
			}

		}
		else if(v_stop)
		{
			Stop();
		}
		else
		{
			LaserMiss_send = false;
			//std::cout<<"State_Machine(p_state_) "<<p_state_<<std::endl;
			State_Machine(p_state_);
		}
	}
}
void alldir::ClearCallback(const std_msgs::Int8& msg)
{
	int obs_clear_buf = msg.data;
	std::vector<SUB_MISSONPATH_SUBPOINT> zzz;

	switch (obs_clear_buf) {
		case Clear_Mode:

			int xxx;
			p_state_ = P_STATE_IDLE;
			if(A_misson.size() > 0)
				A_misson[ready_path_index].ALL_pathnode.clear();

			A_misson.clear();


			memset(&trafficGO_recv,0,sizeof(trafficGO_recv));

			now_A_misson=0;
			now_path_index=0;
			ready_path_index=0;
			Tracking_Angle_Init(xxx, true);
			Tracking_Trajectory(xxx, true);
			Trajectory_Tracking(xxx, true);
			Navigation_move(zzz,true);
			Misson_state(true);

			AvoidObs_state = P_OBS_NORMAL;
			obs_way_theta = 0.0;
			isAvoidObs = false;
			OBS_limilMode = false;
			all_obs_dis = 0.0;
			decay_obs_v = 0.0;
			isFindObs = false;
			last_subpath_index = 0;
			isblind = false;
			avoid_way = 0;
			obs_avoid_flag =false;
			ChangToTraffic=false;
		    ChangToTraffic_finishstop=false;
			isCloseNow = true;
			command_OBSMode = 0;
			obs_return =false;          //閉障回來
			avoid_path.clear();

			protect_erase = true;
			traffic_send = false;


			break;
		case Close_AllObs:
			command_OBSMode = 1;

			break;
		case limitObs_Mode:
			command_OBSMode = 2;

			break;
		case Temporary_Mode:
				int xxxx;
				p_state_ = P_STATE_TEMPORARY;
				if(A_misson.size() > 0)
					A_misson[ready_path_index].ALL_pathnode.clear();

				A_misson.clear();


				memset(&trafficGO_recv,0,sizeof(trafficGO_recv));

				now_A_misson=0;
				now_path_index=0;
				ready_path_index=0;
				Tracking_Angle_Init(xxxx, true);
				Tracking_Trajectory(xxxx, true);
				Trajectory_Tracking(xxxx, true);
				Navigation_move(zzz,true);

				AvoidObs_state = P_OBS_NORMAL;
				obs_way_theta = 0.0;
				isAvoidObs = false;
				OBS_limilMode = false;
				all_obs_dis = 0.0;
				decay_obs_v = 0.0;
				isFindObs = false;
				last_subpath_index = 0;
				isblind = false;
				avoid_way = 0;
				obs_avoid_flag =false;
				ChangToTraffic=false;
					ChangToTraffic_finishstop=false;
				isCloseNow = true;
				command_OBSMode = 0;
				obs_return =false;          //閉障回來
				avoid_path.clear();

				protect_erase = true;
				traffic_send = false;
				break;
		default:
			break;
	}


	std::cout<<"A_misson.size  clear"<<A_misson.size() <<std::endl;
}

void alldir::State_Machine(int& state)
{
int type,id;
	switch (state) {

		case P_STATE_TEMPORARY:
			Command_STATE = Command_STATE_TemporaryMode;
			Idle();
			break;

		case P_STATE_IDLE:
			Command_STATE = Command_STATE_IDLE;

			Idle();

			break;
		case P_STATE_MISSON:
			Misson_state(false);


			break;

		case P_STATE_MOVE:
			//當不是交管處理中
			//找末點模式與哪個id
			 type = A_misson[now_A_misson].sub_missonPath[now_path_index].end_type;
			 id = A_misson[now_A_misson].sub_missonPath[now_path_index].end;


			if(!traffic_send)
			{
				int id = A_misson[now_A_misson].sub_missonPath[now_path_index].end;
				Command_STATE = Command_STATE_TRACKING;
				Command_id = id;
			}

			//交管排隊停車
			if (trafficGO_recv.GO ==2) {
				if(type == Command_STATE_Traffic || type == Command_STATE_VirtualTraffic)
				{
					Stop();
					v_buf = 0.04;
				}

			}
			else
			{
				//std::cout<<"NAV _ last p "<<A_misson[now_A_misson].sub_missonPath[now_path_index].sub_missonPath_subPoint[A_misson[now_A_misson].sub_missonPath[now_path_index].sub_missonPath_subPoint.size()-1]<<std::endl;;
				//導航
				Navigation_move(A_misson[now_A_misson].sub_missonPath, false);


				std::cout<<"type   "<<  type  <<std::endl;
				std::cout<<"id     "<<id<<std::endl;

				if(type == Command_STATE_Traffic)
				{
					if(traffic_send)
					{
						Command_STATE = Command_STATE_Traffic;
						if(now_path_index+1 < A_misson[now_A_misson].sub_missonPath.size())
						{
							//把陣列合併
							if(trafficGO_recv.id==id)
								if(trafficGO_recv.GO == 1)
								{

									A_misson[now_A_misson].sub_missonPath[now_path_index].start = A_misson[now_A_misson].sub_missonPath[now_path_index+1].start;
									A_misson[now_A_misson].sub_missonPath[now_path_index].end_type = A_misson[now_A_misson].sub_missonPath[now_path_index+1].end_type;
									A_misson[now_A_misson].sub_missonPath[now_path_index].end = A_misson[now_A_misson].sub_missonPath[now_path_index+1].end;
									A_misson[now_A_misson].sub_missonPath[now_path_index].sub_missonPath_subPoint.insert(A_misson[now_A_misson].sub_missonPath[now_path_index].sub_missonPath_subPoint.end(),A_misson[now_A_misson].sub_missonPath[now_path_index+1].sub_missonPath_subPoint.begin(),A_misson[now_A_misson].sub_missonPath[now_path_index+1].sub_missonPath_subPoint.end());
									A_misson[now_A_misson].sub_missonPath.erase(A_misson[now_A_misson].sub_missonPath.begin() + now_path_index + 1);
									trafficGO_recv.GO = 0;
									OBS_limilMode = false;
									traffic_send = false;

								}
						}
					}
				}
			}


			break;

		case P_STATE_STOP:

			Stop();

			break;


		case P_STATE_TIME_DELAY:  //LOADING TIME

			if(time_delay_counter >= time_delay_limit){
				time_delay_counter = 0;
				ROS_INFO(" Success!!!\n");
				p_state_ = P_STATE_MOVE;

				int id = A_misson[now_A_misson].sub_missonPath[now_path_index].end;
				for(int i=0;i<A_misson[now_A_misson].ALL_pathnode.size();i++){
					if(A_misson[now_A_misson].ALL_pathnode[i].id==id)
					{
						A_misson[now_A_misson].ALL_pathnode.erase(A_misson[now_A_misson].ALL_pathnode.begin() ,A_misson[now_A_misson].ALL_pathnode.begin()+i);
						break;
					}
            	}






				if(ChangToTraffic)
				{
					std::cout<<"delete ChangToTraffic"<<std::endl;
						p_state_ = P_STATE_MISSON;
						ChangToTraffic_finishstop = true;
						ChangToTraffic = false;
				}

				if(ready_path_index >= A_misson.size())
				{
					ready_path_index =0;
					A_misson[ready_path_index].ALL_pathnode.clear();
					A_misson.clear();
					p_state_=P_STATE_IDLE;
					isALLfinish =false;
					ChangToTraffic_finishstop = false;
					ROS_INFO("All Success!!!\n");
					now_A_misson = 0;

				}

			}
			else{
				time_delay_counter += 1;
				std::cout<<"time_delay_counter   "<<time_delay_counter<<std::endl;
			}

			break;

		case P_STATE_AVOID_WAIT:

			WaitObsLeave();

			break;

		case P_STATE_AVOID_REPLANNING:

			//Stop();

			break;


		case P_STATE_AVOID_OBS:

			AvoidObs();

			break;


		case P_STATE_AVOID_DEADLOCK:

			WaitDeadlock();

			break;


		default:
			break;
	}


}

bool alldir::Navigation_move(std::vector<SUB_MISSONPATH_SUBPOINT> & myAllSubPath, bool isReSet)
{

	static int path_index = 0;
    bool confirm_OBS_limilMode = false;
	static int cnt = 0;

	float delay_time = 0.1;
	int cnt_limit = int(delay_time/time_sample);

	if(!isReSet){
		if(myAllSubPath.size() > 0){

			if(AvoidObs_state == P_OBS_NORMAL && isFindObs == true && cnt >= cnt_limit && !isCloseNow ){
				//進避障

				cnt = 0;
				if(OBS_limilMode == true)
				{
					confirm_OBS_limilMode = true;
				}
				else
				{
					confirm_OBS_limilMode = false;
				}
				Trajectory_Tracking(path_index, true);
				if(confirm_OBS_limilMode == true)
				{
					OBS_limilMode = true;
				}
				AvoidObs_state = P_OBS_WAIT;
				p_state_ = P_STATE_AVOID_WAIT;

			}
			else{

				if(isFindObs){
					cnt += 1;
				}
				else{
					cnt = 0;
				}

				bool isFInish = false;
				now_path_index=path_index;
				isFInish = Trajectory_Tracking(path_index, false);


				if(isFInish){

					path_index += 1;
					p_state_ = P_STATE_MISSON;

					if(path_index >= myAllSubPath.size()){
						cnt = 0;
						path_index = 0;

						std::vector<unsigned char> command;
						sendreceive.Package_AllDir_encoder(0, theta[0],
								0, theta[1],
								0, theta[3],
								0, theta[2],
								command);
						SendPackage(command);


						ready_path_index += 1;
						now_A_misson=ready_path_index -1;

						return true;

					}

				}
			}


		}
		else{
			ready_path_index=0;
			path_index = 0;
			p_state_ = P_STATE_IDLE;
			cnt = 0;
		}
	}
	else{
		ready_path_index=0;
		path_index = 0;
		Trajectory_Tracking(path_index, true);
		isALLfinish =false;
		cnt = 0;
		now_A_misson = 0;

	}
	last_subpath_index = path_index;
	//std::cout<<"======================last_subpath_index================="<<last_subpath_index<<std::endl;
	return false;

}

void alldir::Idle()
{


	Navigation_move(A_misson[ready_path_index].sub_missonPath, true);

	std::vector<unsigned char> command;

	sendreceive.Package_AllDir_encoder(0, theta[0],
			0, theta[1],
			0, theta[3],
			0, theta[2],
			command);


	SendPackage(command);
}

bool alldir::Trajectory_Tracking(int &subpath_index, bool isReSet)//isReSet是fault動作 //true是不動做
{



	static bool isInitial = true;

	bool isFInish = false;

	bool isreset = isReSet;

	if(!isReSet){

		if(isInitial){

			isInitial = !(Tracking_Angle_Init(subpath_index, isreset));

		}
		else{

			isFInish = Tracking_Trajectory(subpath_index, isreset);

		}

		if(isFInish){
			Tracking_Angle_Init(subpath_index, true);//true清空
			Tracking_Trajectory(subpath_index, true);
			isInitial = true;
			return true;
		}


	}
	else{
		Tracking_Angle_Init(subpath_index, true);
		Tracking_Trajectory(subpath_index, true);
		isInitial = true;
	}

	return false;
}
bool alldir::Tracking_Angle_Init(int &subpath_index, bool isReSet)
{

	static bool isFInish = false;
	static bool isInitial = true;
	static int target_ind = 0;
	static float pre_angular_error = 0;


	float angular_kp = 0.5;
	float angular_kd = 0.1;
	float angular_ki = 0;

	float angular_error = 0;

	float Vx = 0;
	float Vy = 0;
	float W_rw = 0;

	static int cmd_angular_cnt = 0;
	int cmd_angular_cnt_limit = 10;


	std::string kinematics_model;

	Eigen::Vector3f target_pos;

	Eigen::Vector3f robot_pos = slam_pose_;

	if(!isReSet){

		if(isInitial){

			int id = A_misson[ready_path_index].sub_missonPath[subpath_index].start;
			for(int i=0;i<A_misson[ready_path_index].ALL_pathnode.size();i++){
				//std::cout<<"A_misson[ready_path_index].ALL_pathnode[i].id   "<<A_misson[ready_path_index].ALL_pathnode[i].id<<std::endl;
				if(A_misson[ready_path_index].ALL_pathnode[i].id==id)
				{
					kinematics_model = A_misson[ready_path_index].ALL_pathnode[i].kin ;
					break;
				}
			}

			std::cout<<"id   "<<id<<std::endl;

			Command_id=A_misson[ready_path_index].sub_missonPath[subpath_index].end;


			//讓所有輪子歸0
			if(fabs(Rev_odom_t1 - 0) <= 0.1 && fabs(Rev_odom_t2 - 0) <= 0.1 &&
						fabs(Rev_odom_t3 - 0) <= 0.1 && fabs(Rev_odom_t4 - 0) <= 0.1){

				std::cout<<"kinematics_model   "<<kinematics_model<<std::endl;

				if(kinematics_model == "omni"){
					isFInish = false;
					isInitial = true;
					target_ind = 0;
					pre_angular_error = 0;
					isCloseNow = true;
					return true;
				}
				else if(kinematics_model == "diff"){
					isInitial = false;
					isCloseNow = true;

					int now_index = 0;
					float odom_v = 0.5;

					target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size()-1];
					Eigen::Vector3f Key_Point;

					float p_mobile_width = CarWidth;
					float p_mobile_length = CarLength;
					float dis_min = sqrt(p_mobile_width*p_mobile_width + p_mobile_length*p_mobile_length)/2.0 +1.0;

					float x_error = target_pos.x() - robot_pos.x();
					float y_error = target_pos.y() - robot_pos.y();
					float dis_error = sqrt(x_error*x_error + y_error*y_error);

					float goal_angle = atan2((target_pos.y() - robot_pos.y()) , (target_pos.x() - robot_pos.x()));

					angular_error = goal_angle - robot_pos.z();

					if(dis_error < dis_min && fabs(angular_error) <= 1.22)//如果離站點太近 角度小於70度 就跳過 可直接由導航之omni進站
					{
						isFInish = false;
						isInitial = true;
						target_ind = 0;
						pre_angular_error = 0;
						isCloseNow = true;
						return true;
					}
					std::cout<<"A   "<<std::endl;



					target_ind = calc_target_index(robot_pos, odom_v, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);
				}
			}
			else
			{std::cout<<"B   "<<std::endl;
				theta[0] = 0;
				theta[1] = 0;
				theta[2] = 0;
				theta[3] = 0;
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
			}



		}
		else{std::cout<<"C   "<<std::endl;

			float cmd_angular_velocity = 0.0;
			float cmd_angular_velocity_buf = 0.0;
			int now_index = 0;
			float odom_v = 0.4;

			target_ind = calc_target_index(robot_pos, odom_v, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);


			std::cout<<"==============A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size()================="<<A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size()<<std::endl;



			target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind];


			float goal_angle = atan2((target_pos.y() - robot_pos.y()) , (target_pos.x() - robot_pos.x()));
			//draw(TrackingLineMarker , 1.0, 0.0, 0.0, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint);


			angular_error = goal_angle - robot_pos.z();

			//std::cout<<"==============goal_angle==============="<<goal_angle<<std::endl;

// std::vector<Eigen::Vector3f> test;
// test.push_back(robot_pos);
// test.push_back(target_pos);
// 			drawLine(5, 0.0 ,0.0 ,1.0,test);

			if(angular_error>M_PI)
				angular_error=angular_error-2*M_PI;
			else if(angular_error<-M_PI)
				angular_error=angular_error+2*M_PI;

			float angular_p_error = angular_error;
			float angular_d_error = angular_error - pre_angular_error;
			pre_angular_error = angular_error;

			cmd_angular_velocity = angular_kp*angular_p_error + angular_kd*angular_d_error;


			//限制旋轉角速度（大）
			if(fabs(cmd_angular_velocity) > 0.3){
				cmd_angular_velocity_buf = (cmd_angular_velocity/fabs(cmd_angular_velocity))*0.3;
				cmd_angular_cnt += 1;
				if(cmd_angular_cnt > cmd_angular_cnt_limit)
				{
					cmd_angular_cnt = cmd_angular_cnt_limit;
					cmd_angular_velocity = cmd_angular_velocity_buf;
				}
				else
				{
					cmd_angular_velocity = cmd_angular_velocity_buf * cmd_angular_cnt /cmd_angular_cnt_limit;
				}

			}
			//限制旋轉角速度（小）
			if(fabs(cmd_angular_velocity) < 0.05){
				cmd_angular_velocity = (cmd_angular_velocity/fabs(cmd_angular_velocity))*0.05;
			}

			//角度誤差內 完成
			if(fabs(angular_error) < 0.1){
				isFInish = true;
				cmd_angular_velocity = 0;
			}

			Vx = 0;
			Vy = 0;

			W_rw = cmd_angular_velocity;


			Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);


			if(isFInish){
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
			}
		}

		std::vector<unsigned char> command;

		sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
				rpm[1], theta[1],
				rpm[3], theta[3],
				rpm[2], theta[2],
				command
				);

		SendPackage(command);

		std::cout<<"rpm   "<<rpm[0]<<std::endl;


		//        ROS_INFO("=====================================");
		ROS_INFO("Tracking_Angle");
		//        ROS_INFO("---------------");
		//        ROS_INFO("now: %f, %f", robot_pos.x(), robot_pos.y());
		//        ROS_INFO("target: %f, %f", target_pos.x(), robot_pos.y());
		//        ROS_INFO("angle_error: %f", angular_error);
		//        ROS_INFO("---------------");
		//        ROS_INFO("FL_control_theta: %d", FL_control_theta);
		//        ROS_INFO("BL_control_theta: %d", BL_control_theta);
		//        ROS_INFO("FR_control_theta: %d", FR_control_theta);
		//        ROS_INFO("BR_control_theta: %d", BR_control_theta);
		//        ROS_INFO("---------------");
		//        ROS_INFO("FL_control_rpm: %d", FL_control_rpm);
		//        ROS_INFO("BL_control_rpm: %d", BL_control_rpm);
		//        ROS_INFO("FR_control_rpm: %d", FR_control_rpm);
		//        ROS_INFO("BR_control_rpm: %d", BR_control_rpm);

		if(isFInish){
			isFInish = false;
			isInitial = true;
			target_ind = 0;
			pre_angular_error = 0;
			cmd_angular_cnt = 0;
			return true;
		}

	}
	else{
		isFInish = false;
		isInitial = true;
		target_ind = 0;
		pre_angular_error = 0;
		cmd_angular_cnt = 0;
	}

	return false;


}


bool alldir::Tracking_Trajectory(int &subpath_index, bool isReSet)
{

	static int target_ind = 0;
	static bool isInitial = true;
	static bool isFInish = false;
	static bool startmove = true;
	//////////////////////////////////////////////////////////
	static bool Endangle = false;   ///new
	static bool confirm_last_diff_angle = false;
  //////////////////////////////////////////////////////////

	isCloseNow = false;


	float speedup_time_s = speedup * time_sample;
	float V_target = speed;//m/s

	//speed control
	static float pre_angular_velocity_error = 0;
	float angular_velocity_kp = tracking_kp;
	float angular_velocity_kd = tracking_kd;

	//position control
	static float pre_dis_error = 0;
	float v_kp = 0.2;
	float v_kd = 0.05;


	static float pre_angular_error = 0;
	float angular_kp = tracking_kp;
	float angular_kd = tracking_kd;

	float Vx = 0;
	float Vy = 0;
	float W_rw = 0;


	float cmd_velocity = 0.0;
	float cmd_angular_velocity = 0.0;
	float V_sub_target = 0.04;
	float dis_error = 1;
	float angular_error = 1;
	float compare_angular_error = 1;//freeloading 用來比較要正heading還是負heading
	float angular_velocity_error=0;
  float compare_angular_velocity_error=0; //freeloading 停站使用

	std::string kinematics_model;

	Eigen::Vector3f robot_pos, target_pos, obs_turn_pos;

	if(!isReSet){
		std::cout<<"=======Tracking_Trajectory======  "<<std::endl;

		//如果進入排隊 可使用這行來降低上限速度 避免車輛速度不同 導致追撞
		if(trafficGO_recv.GO == 3)
		{
			V_target = trafficGO_recv.speed;
		}

		std::cout<<"=======V_target======  "<<V_target<<std::endl;

		robot_pos = slam_pose_;
		//robot_pos = ukf_pose_;



		int now_index;

		//畫出軌跡再rviz
		draw(TrackingLineMarker , 1.0, 0.0, 0.0, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint);

		std::vector<Eigen::Vector3f> NULL_buf;
		draw(ALLOBSPointMarker , 1.0, 0.0, 0.0, NULL_buf);
		draw(AvoidLineMarker , 1.0, 0.0, 0.0, NULL_buf);
		draw(obs_line_pointMarker , 1.0, 1.0, 0.0, NULL_buf);


		drawLine(obs_LineMarker , 1.0, 0.0, 0.0, NULL_buf);
		drawLine(vertical_lineMarker , 1.0, 0.0, 0.0, NULL_buf);
		//畫出軌跡再rviz

		//看此路段最後一個id點
		int id = A_misson[ready_path_index].sub_missonPath[subpath_index].start;


		//查找這個路段使用的模式與停站方式
		for(int i=0;i<A_misson[ready_path_index].ALL_pathnode.size();i++){
			if(A_misson[ready_path_index].ALL_pathnode[i].id==id)
			{
				kinematics_model = A_misson[ready_path_index].ALL_pathnode[i].kin ;
				break;
			}
			std::cout<< "A_misson[ready_path_index].ALL_pathnode[i].id  " <<A_misson[ready_path_index].ALL_pathnode[i].id<<std::endl;
		}
		// std::cout<<"===id=== "<<id<<std::endl;

		// std::cout<<"===kinematics_model=== "<<kinematics_model<<std::endl;

    /////////////////////////
		int id_index = 0;
		for(int i=0;i<A_misson[ready_path_index].ALL_pathnode.size();i++)
		{
				if(A_misson[ready_path_index].ALL_pathnode[i].id == A_misson[ready_path_index].sub_missonPath[subpath_index].end)
				{
						id_index = i;
						break;
				}
		}

		std::cout<<"ChangToTraffic "<<ChangToTraffic<<std::endl;
		//停車點的模式 ex:loading unloading triffic
		int type = A_misson[ready_path_index].sub_missonPath[subpath_index].end_type;
		if(type == MISSON_FreeLoading || type == MISSON_Loading_Time || type == MISSON_unLoading_Time)
		{
				int last_checktype = -1;
				int next_checktype = -1;

				if(id_index >= 2)
				{
						last_checktype = A_misson[ready_path_index].ALL_pathnode[id_index-2].type;
				}
				if(id_index + 2 < A_misson[ready_path_index].ALL_pathnode.size())
				{
						next_checktype = A_misson[ready_path_index].ALL_pathnode[id_index+2].type;
				}

				// std::cout<<"last_checktype "<<last_checktype<<std::endl;
				// std::cout<<"next_checktype "<<next_checktype<<std::endl;

				if(last_checktype == MISSON_traffic || next_checktype == MISSON_traffic || last_checktype == MISSON_Virtual_traffic || next_checktype == MISSON_Virtual_traffic)
				{
						ChangToTraffic = true;

						std::cout<<"ChangToTraffic = true"<<std::endl;
				}
		}

		//////////////////////////
		//查找目前所應追尋的點
		target_ind = calc_target_index(robot_pos, Rev_odom_v, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);
		//目標點
		target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind];






		//===if(startmove)===
		//再啟動時 閉障方向為正確路徑方向
		//做此目的是為了避免啟動時我不再路徑上 我想移動至路線時 我的避障方向如果為運動方向 這個方向移動時有牆壁會導致進入避障問題（不應該出現）
		if(startmove)
		{
			//將即近距離閉障開啟 避免開始行使之位置本false身離障礙物就已經是正常閉障範圍
			//OBS_limilMode = true;
			//起點的模式 ex:loading unloading triffic
			int start_type = A_misson[ready_path_index].sub_missonPath[subpath_index].start_type;
			std::cout<<"===========startmove start_node type========="<<start_type<<std::endl;
			if(start_type ==  MISSON_traffic || start_type ==  MISSON_Virtual_traffic)
			{
					isCloseNow = false;
			}
			else
			{
					isCloseNow = true;
			}

			float x_error_world = 0.0;
			float y_error_world = 0.0;
			float x_error_robot = 0.0;
			float y_error_robot = 0.0;
			//找尋目前路徑應該行使方向之向量
			//保護 因為使用 +？ 所以有意味的可能
			Eigen::Vector3f finial_target_p ;//找目標點後的點
			if(A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size() > target_ind + 3 )
			{
				finial_target_p << A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind + 3];

				x_error_world = finial_target_p.x() - target_pos.x();
				y_error_world = finial_target_p.y() - target_pos.y();
				x_error_robot = x_error_world*cos(-1*robot_pos.z()) - y_error_world*sin(-1*robot_pos.z());
				y_error_robot = x_error_world*sin(-1*robot_pos.z()) + y_error_world*cos(-1*robot_pos.z());

				//tra.world2robot(finial_target_p,target_pos,x_error_robot,y_error_robot);

				// std::cout<<"finial_target_p "<<finial_target_p<<std::endl;
				// std::cout<<"target_pos "<<target_pos<<std::endl;

				// std::cout<<"x_error_robot "<<x_error_robot<<std::endl;
				// std::cout<<"y_error_robot "<<y_error_robot<<std::endl;
			}
			else//假設沒有目標點後一個點
			{
				finial_target_p << A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind -1];

				x_error_world = target_pos.x() - finial_target_p.x();
				y_error_world = target_pos.y() - finial_target_p.y();
				x_error_robot = x_error_world*cos(-1*robot_pos.z()) - y_error_world*sin(-1*robot_pos.z());
				y_error_robot = x_error_world*sin(-1*robot_pos.z()) + y_error_world*cos(-1*robot_pos.z());

				//tra.world2robot(target_pos,finial_target_p,x_error_robot,y_error_robot);
				std::cout<<"B GGGGGGGGG"<<std::endl;
			}
			//由上面得知的向量可以知道目前路徑之行使方向way_theta
			float way_theta = atan2(y_error_robot, x_error_robot);



			// //如果一開始模式為diff
			// if(kinematics_model == "diff")
			// {
			// 	//如果目標方向（預測轉彎之obs_turn_pos_th）沒有太大
			// 	if(fabs(obs_turn_pos_th) < 0.40)
			// 		obs_way_theta=way_theta;
			// }//如果一開始模式為omni 閉障方向即為way_theta
			// else
			// 	obs_way_theta=way_theta;

			//待檢查
			//閉障方向就是行徑方向
			obs_way_theta=way_theta;
			std::cout<<"============obs_way_theta=============="<<obs_way_theta<<std::endl;


			//計算車子離我的路線多遠
			float m_error = 0.0;
			tra.closeline( finial_target_p, target_pos, robot_pos, m_error);

			//走到起始路徑方向<10公分
			if(fabs(m_error) < 0.1 )
			{
				//如果是差速模式 我的頭朝向要跟路線方向要差不多
				if(kinematics_model == "diff")
				{
					float world_theta = atan2(y_error_world, x_error_world);
					float th_error = world_theta - robot_pos.z();
					std::cout<<"th_error  "<<th_error *180/M_PI<<std::endl;
					if(fabs(th_error)< 0.2)//車頭向量跟路線的向量趨於平行
					{
						//OBS_limilMode = false;
						isCloseNow = false;
						startmove = false ;
					}
				}
				else
				{   //OBS_limilMode = false;
					int cnt_omni_startmove_limit = 0;
					float cnt_omni_startmove_time = 1.0;
					static int cnt_omni_startmove = 0;
					cnt_omni_startmove_limit = cnt_omni_startmove_time / time_sample;
					if(cnt_omni_startmove < cnt_omni_startmove_limit)
					{
						cnt_omni_startmove++;
					}
					else
					{
						cnt_omni_startmove = 0;
						isCloseNow = false;
						startmove = false ;
					}
				}
			}
		}
		//===if(startmove)===

		//如果車速再0的狀況（原因1：初始還沒加速 原因2:搖桿中途有切換） 重新校正輪子方向
		if(v_buf <= 0.04)
		{
			isInitial = true;
		}
		//初始化 使輪子一開始的方向為行駛方向
		if(isInitial){
			// std::cout<< "Rev_odom_t1  " <<Rev_odom_t1<<std::endl;
			// std::cout<< "kinematics_model  " <<kinematics_model<<std::endl;
			if(kinematics_model == "omni")
			{
				float th1_normal = obs_way_theta;
				float th1_change = 0.0;
				float errn_th1 = 0.0;
				float errc_th1 = 0.0;

				ini_way_theta = obs_way_theta;

				Car.closeth(ini_way_theta,Rev_odom_t1);


				if(fabs(Rev_odom_t1 - ini_way_theta) <= 0.1 && fabs(Rev_odom_t2 - ini_way_theta) <= 0.1 &&
						fabs(Rev_odom_t3 - ini_way_theta) <= 0.1 && fabs(Rev_odom_t4 - ini_way_theta) <= 0.1){

					isInitial = false;

				}
			}
			else
			{
				if(fabs(Rev_odom_t1 - 0) <= 0.1 && fabs(Rev_odom_t2 - 0) <= 0.1 &&
						fabs(Rev_odom_t3 - 0) <= 0.1 && fabs(Rev_odom_t4 - 0) <= 0.1){

					isInitial = false;

				}
			}
		}


		//隨時監控使否要轉彎以縮小閉障範圍
		int obs_turn_int = calc_target_index_obsturn(robot_pos, Rev_odom_v, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);

		obs_turn_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[obs_turn_int];


		//預測之目標與目前之目標之夾角obs_turn_pos_th
		float obs_turn_pos_x = 0;
		float obs_turn_pos_y = 0;
		tra.world2robot( obs_turn_pos, robot_pos, obs_turn_pos_x, obs_turn_pos_y);
		float obs_turn_pos_th = atan2(obs_turn_pos_y,obs_turn_pos_x) - obs_way_theta;


		V_sub_target = v_buf;

		// std::cout<<"============start========== "<<std::endl;
		// std::cout<<"v_buf"<< v_buf<<std::endl;
		// std::cout<<"V_sub_target"<< V_sub_target<<std::endl;

		if(!isInitial)
		{
			bool isturn_now = false;
			//預測轉彎
			//夾角大於23度即認為有減速必要
			if(fabs(obs_turn_pos_th) > 0.40)
			{std::cout<<"predict"<<std::endl;
				//縮小閉障範圍
				OBS_limilMode = true;
				V_sub_target = V_sub_target - 0.05*V_sub_target ;

				//使最後速度不低於0.25
				if(V_sub_target < Min_nav_speed)
					V_sub_target = Min_nav_speed;

				//如果車速再0的狀況（原因1：初始還沒加速 原因2:搖桿中途有切換）停的位置有符合預測轉彎之範圍 依然需要加速使其行駛 避免直接速度初始變為0.3
				//將V_sub_target 蓋掉
				if(v_buf < Min_nav_speed )
				{
					v_buf = v_buf + speedup_time_s;
					V_sub_target = v_buf;
					//ROS_INFO("v_buf=====A====: %f", v_buf);
				}

			}
			else//沒有預測轉彎狀況
			{
				//實際當下再轉彎路口
				if(now_index > 5)//至少是有超過出發點 才能計算底下
				{

					Eigen::Vector3f back_pose ;//找自己背後1.6m一個點

					if(now_index >= 40)
						back_pose = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[now_index - 40];
					else
						back_pose = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[0];
					float err_sinth = 0;

					Eigen::Vector3f vec1 = target_pos - robot_pos;
					Eigen::Vector3f vec2 = back_pose - robot_pos;

					vec1.z() = 0.0;
					vec2.z() = 0.0;

					Eigen::Vector3f vec_cross = vec1.cross(vec2);

					float vec1_dis = sqrt(pow(vec1.x(),2) + pow(vec1.y(),2) + pow(vec1.z(),2));
					float vec2_dis = sqrt(pow(vec2.x(),2) + pow(vec2.y(),2) + pow(vec2.z(),2));
					float vec_cross_dis = sqrt(pow(vec_cross.x(),2) + pow(vec_cross.y(),2) + pow(vec_cross.z(),2));
					//用外積公式找sinth
					err_sinth = vec_cross_dis /(vec1_dis * vec2_dis);
					//err_sinth = sqrt(pow((vec1.y()*1-1*vec2.y()),2)+pow((1*vec2.x() -vec1.x()*1),2)+pow((vec1.x()*vec2.y() -vec1.y()*vec2.x()),2)) / (sqrt(pow(vec1.x(),2) + pow(vec1.y(),2) + 1) * sqrt(pow(vec2.x(),2) + pow(vec2.y(),2) + 1 )) ;

					//如果夾角th 超過45度 小於135度 即視為轉角

					std::cout<<"err_sinth  "<<err_sinth<<std::endl;
					if(fabs(err_sinth) > 0.707)
					{std::cout<<"turn"<<std::endl;
						isturn_now = true;
						OBS_limilMode = true;

						V_sub_target = V_sub_target - 0.03*V_sub_target ;


						if(V_sub_target < Min_nav_speed)
							V_sub_target = Min_nav_speed;
					}
					else
						OBS_limilMode = false;

				}

				//不會開啟極限避障礙模式

				//obs_return
				// if(!obs_return)
				// {

				//正常加速 可加速到速度上限
				if(isturn_now)
				{
					if(v_buf + speedup_time_s >= V_target )
						V_sub_target = V_target;
					else
					{
						v_buf = v_buf + speedup_time_s;
						V_sub_target = v_buf;
						//ROS_INFO("v_buf=====NORMAL====: %f", v_buf);
					}

				}


				//}
			}
		}

		//std::cout<<"AA"<<std::endl;

		int Path_size = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size();
		//===後段減速====
		//路線很短的情況
		//再剩下30%路徑時開始減速
		int speed_down_count =(int)(Path_size*0.3);
		int speed_down_road = Path_size - speed_down_count;

		//===前85% 的導航====
		int First_tracking = Path_size * 0.2;
std::cout<<"AA"<<std::endl;
		//3m 以上之路徑 3m時會開始減速 1.5m時進入進站模式
		if(Path_size > 75)
		{
			speed_down_count = 75;
			speed_down_road= Path_size - speed_down_count;
			First_tracking = Path_size - 30;
		}

		//路線很長的情況
		//3m時會開始減速 進入 進站模式減速 之穩定減速
		if(now_index > speed_down_road){
			V_sub_target = Min_nav_speed + (V_sub_target - Min_nav_speed)*double(speed_down_count - (now_index - speed_down_road))/double(speed_down_count);
			if(V_sub_target < Min_nav_speed)
				V_sub_target = Min_nav_speed;
			std::cout<<"V_sub_target=====go state==="<< V_sub_target<<std::endl;
		}
		//===後段減速====

		//===交管詢問====
		//如果下一個站點是交管點
		if(type == MISSON_traffic)
		{
			Eigen::Vector3f finial_traffic_p ;

			finial_traffic_p <<A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 1];

			float diserror_traffic = getDistance(finial_traffic_p.x(), finial_traffic_p.y(), robot_pos.x(), robot_pos.y());
			//離交管點< traffic_dis 發送交管消息 尋求是否能直接通過
			if(diserror_traffic < traffic_dis)
				traffic_send = true;
		}

		//===交管詢問====


		//===前85% 的導航====
		if(now_index < First_tracking ){
			std::cout<<"First_tracking"<<std::endl;
			//Angular velocity PID

			float  angular_velocity_p_error = 0.0, angular_velocity_d_error = 0.0;

			//PID只是再追尋軌跡 計算出來為w 跟速度無關
			if(kinematics_model == "diff"){
				//要再正負180內
				angular_velocity_error = atan2((target_pos.y() - robot_pos.y()) , (target_pos.x() - robot_pos.x())) - robot_pos.z();
				if(fabs(angular_velocity_error) > M_PI){
					if(angular_velocity_error > 0) angular_velocity_error = angular_velocity_error - 2*M_PI ;
					else                           angular_velocity_error = angular_velocity_error + 2*M_PI;
				}
				angular_velocity_p_error = angular_velocity_error;
				angular_velocity_d_error = angular_velocity_error - pre_angular_velocity_error;
				pre_angular_velocity_error = angular_velocity_error;
				cmd_angular_velocity = angular_velocity_kp*angular_velocity_p_error + angular_velocity_kd*angular_velocity_d_error;

				if(fabs(cmd_angular_velocity) >= 0.3){
					if(cmd_angular_velocity > 0) cmd_angular_velocity = 0.3;
					else cmd_angular_velocity = -1*0.3;
				}

				//w過大會減速（意味可能有再轉彎）
				if(v_buf > Min_nav_speed )
					if(fabs(angular_velocity_error) > 0.2)//如果我的w大於0.1的話 會持續減速
					{
						V_sub_target = V_sub_target - 0.01*V_sub_target ;


						if(V_sub_target < Min_nav_speed)
							V_sub_target = Min_nav_speed;

						// ROS_INFO("V_sub_target: %f", V_sub_target);
						// ROS_INFO("cmd_angular_velocity: %f", cmd_angular_velocity);
						// std::cout<<"w > 0.2 "<<std::endl;

					}

			}
			else if(kinematics_model == "omni"){
				//觀察停站點應該的方向 邊走邊旋轉成停站點的方向
				Eigen::Vector3f finial_target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 1];
				angular_velocity_error = finial_target_pos.z() - robot_pos.z();
				if(fabs(angular_velocity_error) > M_PI){
					if(angular_velocity_error > 0) angular_velocity_error = angular_velocity_error - 2*M_PI ;
					else                           angular_velocity_error = angular_velocity_error + 2*M_PI;
				}
				angular_velocity_p_error = angular_velocity_error;
				angular_velocity_d_error = angular_velocity_p_error - pre_angular_error;
				pre_angular_error = angular_velocity_p_error;
				cmd_angular_velocity = angular_kp*angular_velocity_p_error + angular_kd*angular_velocity_d_error;

				if(fabs(cmd_angular_velocity) >= 0.2){
					if(cmd_angular_velocity > 0) cmd_angular_velocity = 0.2;
					else cmd_angular_velocity = -1*0.2;
				}

				//
				if(type == MISSON_FreeLoading || type == MISSON_ChangeMode || type == MISSON_traffic || type == MISSON_Virtual_traffic)
				{
					cmd_angular_velocity = 0.0;

					std::cout<<" =======FIRST   cmd_angular_velocity = 0 ==============="<<cmd_angular_velocity<<std::endl;
				}


			}


			//看到障礙物減速 速度要高於XX才實行 減完之速度也會大於Min_nav_speed
			if(V_sub_target > Min_nav_speed  ){
				V_sub_target = V_sub_target - V_sub_target * decay_obs_v;

				std::cout<<"decay_obs_v   "<< decay_obs_v <<std::endl;


				if(V_sub_target < Min_nav_speed)
							V_sub_target = Min_nav_speed;
			}


			if(kinematics_model == "omni"){

				float x_error_robot = 0.0;
				float y_error_robot = 0.0 ;
				tra.world2robot(target_pos,robot_pos,x_error_robot,y_error_robot);

				//這邊跟上面的way_theta概念不太一樣 這邊指真正的位移方向
				float way_theta = atan2(y_error_robot, x_error_robot);

				//控制車子運動
				cmd_velocity = V_sub_target;
				Vx = cmd_velocity*cos(way_theta);
				Vy = cmd_velocity*sin(way_theta);
				W_rw = cmd_angular_velocity;




				//閉障完要回到自己原本的路線
				if(obs_return)
				{//閉完障回來閉障方向要固定再原本閉障方向（omni） obs_way_theta是不改變的 由於回到原路徑與startmove有相同道理 可能會有牆壁 故閉障
					Eigen::Vector3f finial_target_p ;
					finial_target_p << A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind-3];

					float m_error = 0.0;
					tra.closeline( finial_target_p, target_pos, robot_pos, m_error);

					//std::cout<<"m_error "<<m_error <<std::endl;
					//當回到正常路徑時 obs_return = false
					if(fabs(m_error) < 0.05 )
					{
						obs_return = false ;
					}
				}
				else //一般導航狀況進入這裡
				{
					//上面說明 當我離開startmove時 way_theta為真正位移方向當作閉障方向
					if(!startmove)
						obs_way_theta=way_theta;
				}
				//std::cout<<"obs_way_theta  "<<obs_way_theta *180/M_PI<<std::endl;


			}
			else if(kinematics_model == "diff"){
				float way_theta = 0;
				if(!startmove)
				{
					float x_error_robot = 0.0;
					float y_error_robot = 0.0 ;
					tra.world2robot(target_pos,robot_pos,x_error_robot,y_error_robot);

					//這邊指真正的位移方向 由於是diff照裡來說角度誤差應極小 誤差大代表軌跡追蹤精度落差
					way_theta = atan2(y_error_robot, x_error_robot);

					//測試中 直接使用0 (表示必定為車頭方向)    直接使用way_theta (表示應該位移方向)
					obs_way_theta = 0;
					//obs_way_theta=way_theta;
				}



				//保護避免最小角度追逐  導致我的輪子可能轉到另外一邊
				//如果是 V_sub_target == 0 此時還在INI 當我V_sub_target > 0 但速度極小
				if(V_sub_target < Min_nav_speed && V_sub_target > 0)
				{
					if(fabs(cmd_angular_velocity) >= 0.2){
						if(cmd_angular_velocity > 0) cmd_angular_velocity = 0.2;
						else cmd_angular_velocity = -1*0.2;
					}
				}



				cmd_velocity = V_sub_target;
				Vx = cmd_velocity;
				Vy = 0;
				W_rw = cmd_angular_velocity;

				//閉障完要回到自己原本的路線
				//閉障回來時（diff）縮短閉障距離 因為與omni不同 是車頭正在再移動 所以是需要縮短避障距離
				if(obs_return)
				{
					Eigen::Vector3f finial_target_p ;
					finial_target_p <<A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind - 3];

					float m_error = 0.0;
					tra.closeline( finial_target_p, target_pos, robot_pos, m_error);
					OBS_limilMode =true;
					if(fabs(m_error) < 0.08 )
					{
						obs_return = false ;
						OBS_limilMode = false;
					}
				}

			}

			Eigen::Vector3f finial_target_p_gostate(0.0 ,0.0, 0.0) ;

			finial_target_p_gostate = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 1];

			//std::cout<<"finial_target_p_gostate  "<< finial_target_p_gostate<<std::endl;

			float p_mobile_width = CarWidth;
			float p_mobile_length = CarLength;
			float dis_min = sqrt(p_mobile_width*p_mobile_width + p_mobile_length*p_mobile_length)/2.0 + 1.0;

			float x_error = finial_target_p_gostate.x() - robot_pos.x();
			float y_error = finial_target_p_gostate.y() - robot_pos.y();
			dis_error = sqrt(x_error*x_error + y_error*y_error);


			if(type == MISSON_traffic ||type == MISSON_Virtual_traffic || type == MISSON_ChangeMode)
			{}
			else
			{
				if(dis_error <= dis_min)
				{
					isCloseNow = true;
					std::cout<<"GO state" <<std::endl;
				}

			}




			if(!isInitial)
				v_buf = V_sub_target;

			Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

			// std::cout<<"============leave========== "<<std::endl;
			// std::cout<<"W_rw "<<W_rw<<std::endl;
			// std::cout<<"V_sub_target "<<V_sub_target<<std::endl;
			// std::cout<<"Vx "<<Vx<<std::endl;
			// std::cout<<"Vy "<<Vy<<std::endl;


		}//===前85% 的導航====
		//===要進站====
		else{

			////////////////////////////////////////////////////////////////////////////////////////
			std::cout<<"=======Last_tracking========"<<std::endl;
			robot_pos = slam_pose_;
			if(type == MISSON_traffic ||type == MISSON_Virtual_traffic || type == MISSON_ChangeMode)
			{
				isCloseNow = false;
				OBS_limilMode = true;
			}
			else
			{
				isCloseNow = true;
			}
			////////////////////////////////////////////////////算V
			target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size() - 1];

			std::cout<<"Last_tracking target_pos "<<target_pos<<std::endl;

			float x_error = target_pos.x() - robot_pos.x();
			float y_error = target_pos.y() - robot_pos.y();

			float x_error_robot = x_error*cos(-1*robot_pos.z()) - y_error*sin(-1*robot_pos.z());
			float y_error_robot = x_error*sin(-1*robot_pos.z()) + y_error*cos(-1*robot_pos.z());
			float way_theta = atan2(y_error_robot, x_error_robot);


			dis_error = sqrt(x_error*x_error + y_error*y_error);
			float dis_p_error = dis_error;
			float dis_d_error = dis_error - pre_dis_error;
			pre_dis_error = dis_error;

			cmd_velocity = v_kp*dis_p_error + v_kd*dis_d_error;

			if(cmd_velocity > Min_nav_speed){
				cmd_velocity = Min_nav_speed;
			}
			else if(cmd_velocity < 0.15){
				cmd_velocity = 0.15;
			}

			float cmd_vx = 0;
			float cmd_vy = 0;
			///////////////////////////////////////////////////////////////////////////////
			if(kinematics_model == "diff")
			{
					angular_error = target_pos.z() - robot_pos.z();
					if(fabs(angular_error) > M_PI){
						if(angular_error > 0) angular_error = angular_error - 2*M_PI ;
						else                  angular_error = angular_error + 2*M_PI;
					}
					if(type == MISSON_traffic ||type == MISSON_Virtual_traffic)
					{
						Eigen::Vector3f  target_pos_last = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 2];
						float stop_angle = atan2((target_pos.y() - target_pos_last.y()) , (target_pos.x() - target_pos_last.x()));
						angular_error = stop_angle - robot_pos.z();

						if(fabs(angular_error) > M_PI)
						{
							if(angular_error > 0) angular_error = angular_error - 2*M_PI ;
							else                  angular_error = angular_error + 2*M_PI;
						}
					}
					//做判斷如果跟heading方向差60度以上diff到點後轉回heading方向  沒有就跟原本一樣
					std::cout<<"==========last_tracking_diff_angular_error=========="<<fabs(angular_error)<<std::endl;
					if(fabs(angular_error) >= ((60.0/180.0)*M_PI))
					{
						  std::cout<<"last station diff angle over 60"<<std::endl;
						  confirm_last_diff_angle = true;

							cmd_vx = cmd_velocity;
							cmd_vy = 0;

							//Eigen::Vector3f  target_pos_last = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 2];
							float stop_angle = atan2((target_pos.y() - robot_pos.y()) , (target_pos.x() - robot_pos.x()));
							Caculate_W_rw(stop_angle, robot_pos, angular_velocity_error, pre_angular_velocity_error, cmd_angular_velocity, false);

							angular_error = angular_velocity_error;

					}
					else //omni模式
					{
							std::cout<<"last station diff angle small than 60"<<std::endl;
							cmd_vx = cmd_velocity*cos(way_theta);
							cmd_vy = cmd_velocity*sin(way_theta);
							//非這兩種情況 依照原定heading停車
							if(type == MISSON_FreeLoading)
							{
									Caculate_W_rw(target_pos.z(), robot_pos, angular_velocity_error, pre_angular_velocity_error, cmd_angular_velocity, true);

							 		angular_error = angular_velocity_error;
							}
							else if(type == MISSON_traffic ||  type == MISSON_Virtual_traffic)
							{
									Eigen::Vector3f  target_pos_last = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 2];
									float stop_angle = atan2((target_pos.y() - target_pos_last.y()) , (target_pos.x() - target_pos_last.x()));
									Caculate_W_rw(stop_angle, robot_pos, angular_velocity_error, pre_angular_velocity_error, cmd_angular_velocity, false);
									angular_error = angular_velocity_error;
							}
							else
							{
									Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, false);
							}
					}
			}
			else   //omni mode
			{
				  std::cout<<"last station omni"<<std::endl;
					cmd_vx = cmd_velocity*cos(way_theta);
					cmd_vy = cmd_velocity*sin(way_theta);
					//非這兩種情況 依照原定heading停車
					if(type == MISSON_FreeLoading)
					{
						    //任意方向都可停
								// W_rw = 0;
								// angular_error = 0.0;

								std::cout<<"==============target_pos.z()============="<<target_pos.z()<<std::endl;

								Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, true);
								std::cout<<"==============normal_last_station_omni_freeloading============="<<cmd_angular_velocity<<std::endl;

					}
					else if(type == MISSON_traffic ||  type == MISSON_Virtual_traffic)
					{
							//Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, true);
							cmd_angular_velocity = 0.0;
							angular_error = 0.0;
					}
					else
					{
								std::cout<<"==============normal_last_station_omni============="<<std::endl;
								Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, false);

					}
			}



			//protect
			if(fabs(cmd_angular_velocity) >= 0.4){
				if(cmd_angular_velocity > 0) cmd_angular_velocity = 0.4;
				else cmd_angular_velocity = -1*0.4;
			}
			Vx = cmd_vx;
			Vy = cmd_vy;
			W_rw = cmd_angular_velocity;

			Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

			//std::cout<<"==============send_Vx "<<

			v_buf = cmd_velocity;
			//std::cout<<"===============last_Vx============"<<Vx<<std::endl;
      //std::cout<<"===============last_Vy============"<<Vy<<std::endl;
      //std::cout<<"===============last_W============"<<W_rw<<std::endl;



		}
// 		std::cout<<"===============trafficGO_recv.id============"<<trafficGO_recv.id<<std::endl;
// 		std::cout<<"===============trafficGO_recv.GO============"<<trafficGO_recv.GO<<std::endl;

// std::cout<<"===============id==========="<<id<<std::endl;



		if(Endangle)
		{
			int id_ = A_misson[ready_path_index].sub_missonPath[subpath_index].end;

				std::cout<<"last station Endangle"<<std::endl;
				if(trafficGO_recv.id==id_)
				{
					if(trafficGO_recv.GO == 1)
					{
						std::cout<<"endangle trafficGO_recv"<<std::endl;
						Endangle = false;
					}
				}
				//非這兩種情況 依照原定heading停車
				if(type == MISSON_FreeLoading )
				{
					  Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, true);
					  std::cout<<"============Endangle_MISSON_FreeLoading angular_error========="<<angular_error<<std::endl;
					  std::cout<<"==============================Endangle_MISSON_FreeLoading=========================="<<std::endl;
				}
				// else if(type == MISSON_traffic ||  type == MISSON_Virtual_traffic)
				// {
				// 		Eigen::Vector3f  target_pos_last = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[Path_size - 2];
				// 		float stop_angle = atan2((target_pos.y() - target_pos_last.y()) , (target_pos.x() - target_pos_last.x()));
				// 		Caculate_W_rw(stop_angle, robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, false);
				// }
				else
				{
						Caculate_W_rw(target_pos.z(), robot_pos, angular_error, pre_angular_error, cmd_angular_velocity, false);
				}
				//protect
				if(fabs(cmd_angular_velocity) >= 0.3){
					if(cmd_angular_velocity > 0) cmd_angular_velocity = 0.3;
					else cmd_angular_velocity = -1*0.3;
				}
				Vx = 0;
				Vy = 0;
				W_rw = cmd_angular_velocity;
				Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
				if(fabs(angular_error) <= 0.05)
				{
						isFInish = true;
						cmd_velocity = 0;
						W_rw = 0;
						Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
				}
		}

		if(confirm_last_diff_angle  && dis_error <= 0.03 && !Endangle )
		{
				std::cout<<"==============================let endangle true=========================="<<std::endl;
				Endangle = true;
				cmd_velocity = 0;
				W_rw = 0;
				Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
				angular_error = 100.0;
		}

		if(Endangle == true && fabs(angular_error) <= 0.03)
		{
				isFInish = true;
				cmd_velocity = 0;
				W_rw = 0;
				Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
		}

		if(isInitial){
			if(kinematics_model == "omni")
			{
				theta[0] = ini_way_theta*1800/M_PI;
				theta[1] = ini_way_theta*1800/M_PI;
				theta[2] = ini_way_theta*1800/M_PI;
				theta[3] = ini_way_theta*1800/M_PI;

				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
				std::cout<<"isInitial "<<std::endl;
			}
			else
			{
				theta[0] = 0;
				theta[1] = 0;
				theta[2] = 0;
				theta[3] = 0;

				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
				std::cout<<"isInitial "<<std::endl;
			}
		}
		else if(type == MISSON_ChangeMode && !confirm_last_diff_angle)
		{
			if(fabs(angular_error) <= ChangeMode_angular_error && dis_error <= ChangeMode_dis_error){
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
				isFInish = true;
				std::cout<<"isFInish ChangeMode"<<std::endl;
			}

		}
		else if((type == MISSON_traffic ||type == MISSON_Virtual_traffic) && !confirm_last_diff_angle)
		{
			if( dis_error <= traffic_dis_error){
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;
				isFInish = true;
				std::cout<<"isFInish traffic"<<std::endl;
			}

		}
		else if(fabs(angular_error) <= othermode_angular_error && dis_error <= othermode_dis_error && !confirm_last_diff_angle){
			rpm[0] = 0;
			rpm[1] = 0;
			rpm[2] = 0;
			rpm[3] = 0;
			isFInish = true;
			std::cout<<"isFInish  NORMAL "<<std::endl;
		}
// std::cout<<"BBBBB"<<std::endl;

// std::cout<<"angular_error "<< angular_error<<std::endl;
// std::cout<<"dis_error "<< dis_error<<std::endl;
		std::vector<unsigned char> command;

		sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
				rpm[1], theta[1],
				rpm[3], theta[3],
				rpm[2], theta[2],
				command
				);

		SendPackage(command);

		//        ROS_INFO("---------------");
		//        ROS_INFO("odom_v: %f", Rev_odom_v);
		//        ROS_INFO("now_index: %d", now_index);
		//        ROS_INFO("target_ind: %d", target_ind);
		//        ROS_INFO("---------------");
		//        ROS_INFO("Robot pose x: %f y: %f yaw: %f", slam_pose_[0], slam_pose_[1], slam_pose_[2]);
		//        ROS_INFO("---------------");
		       ROS_INFO("Vx: %f", Vx);
		       ROS_INFO("Vy: %f", Vy);
		       ROS_INFO("W_rw: %f", W_rw);

		if(isFInish){
			isCloseNow = true;
			obs_return =false;
			target_ind = 0;
			isInitial = true;
			startmove = true;
			isFInish = false;
			OBS_limilMode =false;
			pre_angular_velocity_error = 0;
			pre_dis_error = 0;
			pre_angular_error = 0;
			v_buf = 0.04;
			std::cout<<"isFInish B"<<std::endl;
			traffic_send = false;
			Endangle = false;
			confirm_last_diff_angle = false;

			return true;
		}
	}
		else{
			isCloseNow = true;
			obs_return =false;
			target_ind = 0;
			isInitial = true;
			startmove = true;
			isFInish = false;
			OBS_limilMode =false;
			pre_angular_velocity_error = 0;
			pre_dis_error = 0;
			pre_angular_error = 0;
			v_buf = 0.04;
			traffic_send = false;
			Endangle = false;
			confirm_last_diff_angle = false;
		}
		return false;
}
void alldir::laserCallback(const sensor_msgs::LaserScan& scan)
{

		ros::Duration dur (0.5);

		LaserMiss = 0;
		//std::cout<<"ONE LASER"<<std::endl;
		//Receive LaserData
		int device_num = checkDeviceNum(scan);

		sensor_msgs::LaserScan device_scan_1;
		sensor_msgs::LaserScan device_scan_2;

		if(device_num == 1){

			device_scan_1.header          = scan.header;
			device_scan_1.angle_min       = scan.angle_min;
			device_scan_1.angle_max       = scan.angle_max;
			device_scan_1.angle_increment = scan.angle_increment;
			device_scan_1.time_increment  = scan.time_increment;
			device_scan_1.scan_time       = scan.scan_time;
			device_scan_1.range_min       = scan.range_min;
			device_scan_1.range_max       = scan.range_max;
			device_scan_1.ranges.resize(scan.ranges.size());
			device_scan_1.intensities.resize(scan.ranges.size());

			for(int i=0; i<scan.ranges.size(); i++)
				device_scan_1.ranges[i] = scan.ranges[i];

		}
		else{

			device_scan_1.header          = scan.header;
			device_scan_1.angle_min       = scan.angle_min;
			device_scan_1.angle_max       = scan.angle_max;
			device_scan_1.angle_increment = scan.angle_increment;
			device_scan_1.time_increment  = scan.time_increment;
			device_scan_1.scan_time       = scan.scan_time;
			device_scan_1.range_min       = scan.range_min;
			device_scan_1.range_max       = scan.range_max;
			device_scan_1.ranges.resize(scan.ranges.size()/2);
			device_scan_1.intensities.resize(scan.ranges.size()/2);

			device_scan_2.header          = scan.header;
			std::string frame_id_2_       = std::string("base_laser_link_2");
			device_scan_2.header.frame_id = frame_id_2_;
			device_scan_2.angle_min       = scan.angle_min;
			device_scan_2.angle_max       = scan.angle_max;
			device_scan_2.angle_increment = scan.angle_increment;
			device_scan_2.time_increment  = scan.time_increment;
			device_scan_2.scan_time       = scan.scan_time;
			device_scan_2.range_min       = scan.range_min;
			device_scan_2.range_max       = scan.range_max;
			device_scan_2.ranges.resize(scan.ranges.size()/2);
			device_scan_2.intensities.resize(scan.ranges.size()/2);


			for(int i=0; i<(scan.ranges.size()/2); i++){
				device_scan_1.ranges[i] = scan.ranges[i];
				device_scan_2.ranges[i] = scan.ranges[i+(scan.ranges.size()/2)];
			}
		}


		if (tf_.waitForTransform(p_base_frame_,device_scan_1.header.frame_id, device_scan_1.header.stamp,dur))
		{

			tf::StampedTransform laserTransform_1;
			tf_.lookupTransform(p_base_frame_,device_scan_1.header.frame_id, device_scan_1.header.stamp, laserTransform_1);

			if(device_num == 2)
			{
				if (tf_.waitForTransform(p_base_frame_,device_scan_2.header.frame_id, device_scan_2.header.stamp,dur))
				{
					tf::StampedTransform laserTransform_2;
					tf_.lookupTransform(p_base_frame_,device_scan_2.header.frame_id, device_scan_2.header.stamp, laserTransform_2);

					projector_1_.projectLaser(device_scan_1, laser_point_cloud_1_,20.0);
					projector_2_.projectLaser(device_scan_2, laser_point_cloud_2_,20.0);


					size_t laser_size_1 = laser_point_cloud_1_.points.size();
					size_t laser_size_2 = laser_point_cloud_2_.points.size();

					tf::Vector3 laserPos_1 (laserTransform_1.getOrigin());
					tf::Vector3 laserPos_2 (laserTransform_2.getOrigin());

					std::vector<Eigen::Vector3f> obs_container;

					std::vector<Eigen::Vector3f> obs_region_0; //-90 ~ -54
					std::vector<Eigen::Vector3f> obs_region_1; //-54 ~ -18
					std::vector<Eigen::Vector3f> obs_region_2; //-18 ~ 18
					std::vector<Eigen::Vector3f> obs_region_3; // 18 ~ 54
					std::vector<Eigen::Vector3f> obs_region_4; // 54 ~ 90


					for (size_t i = 0; i < laser_size_1; ++i){

						const geometry_msgs::Point32& currPoint(laser_point_cloud_1_.points[i]);

						float dist_sqr = currPoint.x*currPoint.x + currPoint.y* currPoint.y;

						if ( (dist_sqr > 0.05) && (dist_sqr < 20) ){

							if ( (currPoint.x < 0.0f) && (dist_sqr < 0.2f)){
								continue;
							}

							tf::Vector3 pointPosBaseFrame(laserTransform_1 * tf::Vector3(currPoint.x, currPoint.y, currPoint.z));

							double center_dis = sqrt(pointPosBaseFrame.x()*pointPosBaseFrame.x()+ pointPosBaseFrame.y()*pointPosBaseFrame.y());

							float pointPosLaserFrameZ = pointPosBaseFrame.z() - laserPos_1.z();

							if (pointPosLaserFrameZ > -1 && pointPosLaserFrameZ < 1)
							{
								Eigen::Vector3f p(pointPosBaseFrame.x(), pointPosBaseFrame.y(), pointPosBaseFrame.z());
								obs_container.push_back(p);

							}
						}
					}


					for (size_t i = 0; i < laser_size_2; ++i)
					{

						const geometry_msgs::Point32& currPoint(laser_point_cloud_2_.points[i]);

						float dist_sqr = currPoint.x*currPoint.x + currPoint.y* currPoint.y;

						if ( (dist_sqr > 0.05) && (dist_sqr < 20) ){

							if ( (currPoint.x < 0.0f) && (dist_sqr < 0.2f)){
								continue;
							}

							tf::Vector3 pointPosBaseFrame(laserTransform_2 * tf::Vector3(currPoint.x, currPoint.y, currPoint.z));

							double center_dis = sqrt(pointPosBaseFrame.x()*pointPosBaseFrame.x()+ pointPosBaseFrame.y()*pointPosBaseFrame.y());

							float pointPosLaserFrameZ = pointPosBaseFrame.z() - laserPos_2.z();

							if (pointPosLaserFrameZ > -1 && pointPosLaserFrameZ < 1)
							{

								Eigen::Vector3f p(pointPosBaseFrame.x(), pointPosBaseFrame.y(), pointPosBaseFrame.z());
								obs_container.push_back(p);

							}
						}

					}
					std::vector<Eigen::Vector3f> obs_gap_0;
					std::vector<Eigen::Vector3f> obs_gap_1;
					std::vector<Eigen::Vector3f> obs_gap_2;
					std::vector<Eigen::Vector3f> obs_gap_3;
					std::vector<Eigen::Vector3f> obs_gap_4;
					std::vector<Eigen::Vector3f> all_point;

					//--------------------------------------------//
					if(command_OBSMode != 1){
						obs_region_0.clear();
						obs_region_1.clear();
						obs_region_2.clear();
						obs_region_3.clear();
						obs_region_4.clear();

						ObsDetect(obs_container, obs_region_0, obs_region_1, obs_region_2, obs_region_3, obs_region_4,obs_gap_0,obs_gap_2,obs_gap_4,all_point);

						if(!isReveice_joystick)
						{
							ObsStateMachine(obs_region_0, obs_region_1, obs_region_2, obs_region_3, obs_region_4,obs_gap_0,obs_gap_2,obs_gap_4,all_point);
							obs_container.clear();
						}
					}

				}
			}
			else
			{std::cout<<"ONE LASER"<<std::endl;
				projector_1_.projectLaser(device_scan_1, laser_point_cloud_1_,20.0);

				size_t laser_size_1 = laser_point_cloud_1_.points.size();

				tf::Vector3 laserPos_1 (laserTransform_1.getOrigin());


				std::vector<Eigen::Vector3f> obs_container;


				std::vector<Eigen::Vector3f> obs_region_0; //-90 ~ -54
				std::vector<Eigen::Vector3f> obs_region_1; //-54 ~ -18
				std::vector<Eigen::Vector3f> obs_region_2; //-18 ~ 18
				std::vector<Eigen::Vector3f> obs_region_3; // 18 ~ 54
				std::vector<Eigen::Vector3f> obs_region_4; // 54 ~ 90


				for (size_t i = 0; i < laser_size_1; ++i){

					const geometry_msgs::Point32& currPoint(laser_point_cloud_1_.points[i]);

					float dist_sqr = currPoint.x*currPoint.x + currPoint.y* currPoint.y;

					if ( (dist_sqr > 0.05) && (dist_sqr < 20) ){

						if ( (currPoint.x < 0.0f) && (dist_sqr < 0.2f)){
							continue;
						}

						tf::Vector3 pointPosBaseFrame(laserTransform_1 * tf::Vector3(currPoint.x, currPoint.y, currPoint.z));

						double center_dis = sqrt(pointPosBaseFrame.x()*pointPosBaseFrame.x()+ pointPosBaseFrame.y()*pointPosBaseFrame.y());

						float pointPosLaserFrameZ = pointPosBaseFrame.z() - laserPos_1.z();

						if (pointPosLaserFrameZ > -1 && pointPosLaserFrameZ < 1)
						{
							Eigen::Vector3f p(pointPosBaseFrame.x(), pointPosBaseFrame.y(), pointPosBaseFrame.z());
							obs_container.push_back(p);

						}
					}
				}

				std::vector<Eigen::Vector3f> obs_gap_0;
				std::vector<Eigen::Vector3f> obs_gap_1;
				std::vector<Eigen::Vector3f> obs_gap_2;
				std::vector<Eigen::Vector3f> obs_gap_3;
				std::vector<Eigen::Vector3f> obs_gap_4;
				std::vector<Eigen::Vector3f> all_point;

				//--------------------------------------------//
				if(command_OBSMode != 1){
					obs_region_0.clear();
					obs_region_1.clear();
					obs_region_2.clear();
					obs_region_3.clear();
					obs_region_4.clear();

					ObsDetect(obs_container, obs_region_0, obs_region_1, obs_region_2, obs_region_3, obs_region_4,obs_gap_0,obs_gap_2,obs_gap_4,all_point);

					if(!isReveice_joystick)
					{
						ObsStateMachine(obs_region_0, obs_region_1, obs_region_2, obs_region_3, obs_region_4,obs_gap_0,obs_gap_2,obs_gap_4,all_point);
						obs_container.clear();
					}

				}

			}

		}


	}
	void alldir::ObsDetect(std::vector<Eigen::Vector3f> &i_obs_container,
			std::vector<Eigen::Vector3f> &i_obs_region_0,
			std::vector<Eigen::Vector3f> &i_obs_region_1,
			std::vector<Eigen::Vector3f> &i_obs_region_2,
			std::vector<Eigen::Vector3f> &i_obs_region_3,
			std::vector<Eigen::Vector3f> &i_obs_region_4,
			std::vector<Eigen::Vector3f> &i_obs_gap_0,
			std::vector<Eigen::Vector3f> &i_obs_gap_2,
			std::vector<Eigen::Vector3f> &i_obs_gap_4,
			std::vector<Eigen::Vector3f> &all_point)
	{
		//遇到障礙物減速用

		std::vector<Eigen::Vector3f> i_obs_region_2_buf;


		int obs_num_limit = OBS_isObs;


		float car_warn_theta = fabs(obs_way_theta);
		if(car_warn_theta>M_PI/2)
			car_warn_theta=M_PI-car_warn_theta;


		float Car_Front = CarLength* cos(car_warn_theta)/2 +  CarWidth* sin(car_warn_theta)/2 ;
		Car_Side = CarWidth * cos(car_warn_theta)/2 + CarLength * sin(car_warn_theta)/2;

		//進入避障參數
		float protect_OBS_Side = Car_Side + OBS_Side;
		float protect_OBS_Front = Car_Front + OBS_Front;
		float protect_OBS_LimitFront = Car_Front + OBS_Front_limit;
		float protect_OBS_Front_avoid = Car_Front + (OBS_Front+ OBS_Front_limit)/2; //避障後 閉障距離變短一半 （可不要 可以是必障限制）

		Eigen::Vector3f robot_pos_te = slam_pose_;
		std::fstream fout;
		//fout.open("robot_pos",std::fstream::app);
		//fout<<robot_pos_te.x() <<"\t" <<robot_pos_te.y() <<std::endl;
		//fout<<"====================" <<std::endl;


		//讓雷射點相對座標 轉成相對於車子移動方向
		if(obs_way_theta !=0)
			for(int i=0;i<i_obs_container.size();i++)
			{
				Eigen::Vector3f p_buf = i_obs_container[i];
				i_obs_container[i].x() = p_buf.x() *cos(-1*obs_way_theta) - p_buf.y() * sin(-1*obs_way_theta);
				i_obs_container[i].y() = p_buf.x() *sin(-1*obs_way_theta) + p_buf.y() * cos(-1*obs_way_theta);
				//fout<<i_obs_container[i].x() <<"\t" <<i_obs_container[i].y() <<std::endl;

			}
		//fout<<"====================" <<std::endl;
		//fout.close();
		//std::cout<<"obs_way_theta  "<< obs_way_theta<<std::endl;

		for(int i=0; i<i_obs_container.size(); i++){
			Eigen::Vector3f p = i_obs_container[i];
			//一般避障區間
			if(p.x() < protect_OBS_Front && p.x() > -1*Car_Front){
				if( fabs(p.y()) < OBS_SideMiss_Scale*protect_OBS_Side)
				{

					if(p.y() < -1.0*protect_OBS_Side && p.x() < Car_Front)       i_obs_region_4.push_back(p);
					else if(p.y() < -1.0*protect_OBS_Side && p.x() >= Car_Front)  i_obs_region_3.push_back(p);
					else if(p.x()> -1.0*Car_Front &&  fabs(p.y()) <= protect_OBS_Side )
					{
						if(OBS_limilMode || command_OBSMode==2)//再轉彎時 與 閉障結束回到正常路徑時（diff） 閉障縮短
						{

							if( p.x()<protect_OBS_LimitFront) i_obs_region_2.push_back(p);
							all_obs_dis = protect_OBS_LimitFront;
						}
						else if(isAvoidObs)// 閉障時會把閉障距離縮短
						{

							if( p.x()< protect_OBS_Front_avoid) i_obs_region_2.push_back(p);
							all_obs_dis = protect_OBS_Front_avoid;
						}
						else
						{
							i_obs_region_2.push_back(p);
							all_obs_dis = protect_OBS_Front;

						}

					}
					else if(p.y() > protect_OBS_Side && p.x() >= Car_Front)  i_obs_region_1.push_back(p);
					else if(p.y() > protect_OBS_Side && p.x() < Car_Front)  i_obs_region_0.push_back(p);

				}
			}
			//避障減速區間
			if(p.x() < (all_obs_dis *2) && p.x() > -1*Car_Front){
				if(fabs(p.y()) <= protect_OBS_Side   )i_obs_region_2_buf.push_back(p);
			}
			//gap觀看區間
			if(p.x() < OBS_LookGap && p.x() > -1*Car_Front)
			{
				if(fabs(p.y()) < OBS_LookGap )
				{
					if(p.y() < -1*protect_OBS_Side) i_obs_gap_4.push_back(p);
					else if (fabs(p.y()) <= protect_OBS_Side && p.x() < all_obs_dis * 2.0 ) i_obs_gap_2.push_back(p);
					else if (p.y() > protect_OBS_Side )  i_obs_gap_0.push_back(p);

				}

				all_point.push_back(p);
			}

		}

		//避障減速
		if(i_obs_region_2_buf.size() >= obs_num_limit){
			decay_obs_v = OBS_Decay_V;
		}
		else
		{
			decay_obs_v = 0.0;
		}


		//畫出軌跡再rviz 避障區間內看到的點畫出來
		Eigen::Vector3f robot_pos;
		robot_pos = slam_pose_;
		float i_obs_region_2_buf_x = 0;
		float i_obs_region_2_buf_y = 0;
		std::vector<Eigen::Vector3f> look_front;
		for(int i=0;i<i_obs_region_2.size();i++)
		{
			tra.robot2world(i_obs_region_2[i],robot_pos,obs_way_theta,i_obs_region_2_buf_x,i_obs_region_2_buf_y);

			Eigen::Vector3f p ;
			p <<i_obs_region_2_buf_x , i_obs_region_2_buf_y, 1.0;
			look_front.push_back(p);
		}
		//畫出軌跡再rviz
		draw(OBSPointMarker , 0.0, 1.0, 0.0, look_front);
		//畫出軌跡再rviz

		// std::cout<<"===================" <<std::endl;
		// std::cout<<"robot_pos  " << robot_pos.z()*180.0/M_PI<<std::endl;
		// std::cout<<"obs_way_theta  " << obs_way_theta*180.0/M_PI<<std::endl;
		// std::cout<<"i_obs_region_0  " << i_obs_region_0.size()<<std::endl;
		// std::cout<<"i_obs_region_1  " << i_obs_region_1.size()<<std::endl;
		// std::cout<<"i_obs_region_2  " << i_obs_region_2.size()<<std::endl;
		// std::cout<<"i_obs_region_3  " << i_obs_region_3.size()<<std::endl;
		// std::cout<<"i_obs_region_4  " << i_obs_region_4.size()<<std::endl;
		// std::cout<<"===================" <<std::endl;

	}

	void alldir::ObsStateMachine(std::vector<Eigen::Vector3f> &i_obs_region_0,
			std::vector<Eigen::Vector3f> &i_obs_region_1,
			std::vector<Eigen::Vector3f> &i_obs_region_2,
			std::vector<Eigen::Vector3f> &i_obs_region_3,
			std::vector<Eigen::Vector3f> &i_obs_region_4,
			std::vector<Eigen::Vector3f> &i_obs_gap_0,
			std::vector<Eigen::Vector3f> &i_obs_gap_2,
			std::vector<Eigen::Vector3f> &i_obs_gap_4,
			std::vector<Eigen::Vector3f> &all_point)
	{
		static int cnt = 0;
		int cnt_limit = 20;

		int obs_num_limit = OBS_isObs;

		//std::cout<<"AvoidObs_state  "<< AvoidObs_state<<std::endl;

		if(AvoidObs_state == P_OBS_NORMAL){
			if(i_obs_region_2.size() > obs_num_limit){
				isFindObs = true;
			}
			else{
				isFindObs = false;
			}
		}
		else if(AvoidObs_state == P_OBS_WAIT){
			if(i_obs_region_2.size() > obs_num_limit){
				isFindObs = true;
			}
			else{
				isFindObs = false;
			}
		}
		else if(AvoidObs_state == P_OBS_REPLANNING){
			//isFindObs = false;
			RePlanning(last_subpath_index, i_obs_gap_0,  i_obs_gap_2,  i_obs_gap_4, all_point);
		}
		else if(AvoidObs_state == P_OBS_AVOID){

			if(isblind){

				if(i_obs_region_0.size() < obs_num_limit &&
						i_obs_region_1.size() < obs_num_limit &&
						i_obs_region_2.size() < obs_num_limit &&
						i_obs_region_3.size() < obs_num_limit &&
						i_obs_region_4.size() < obs_num_limit )
				{
					isblind = false;
				}
				else
				{
					isFindObs = false;
					isAvoidObs = true;
				}



			}
			else if(i_obs_region_2.size() >= obs_num_limit){


				isFindObs = true;
				isAvoidObs = false;
				cnt = 0;

				std::cout<<"GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"<<std::endl;

			}
			else if(avoid_way < 0 &&
					i_obs_region_0.size() < obs_num_limit &&
					i_obs_region_1.size() < obs_num_limit){

				if(cnt >= cnt_limit){
					isFindObs = false;
					isAvoidObs = false;
					cnt = 0;

				}
				else{
					isFindObs = false;
					isAvoidObs = true;

					cnt ++;
				}


			}
			else if(avoid_way > 0 &&
					i_obs_region_3.size() < obs_num_limit &&
					i_obs_region_4.size() < obs_num_limit){
				std::cout<<"==============avoid_way >0==============  "<<std::endl;
				if(cnt >= cnt_limit){
					isFindObs = false;
					isAvoidObs = false;
					cnt = 0;

				}
				else{
					isFindObs = false;
					isAvoidObs = true;


					cnt ++;
				}
			}
			else{
				isFindObs = false;
				isAvoidObs = true;
				cnt = 0;
			}



		}
		else if(AvoidObs_state == P_OBS_DEADLOCK){
			if( i_obs_region_2.size() > obs_num_limit ){
				isFindObs = true;
			}
			else if(i_obs_region_0.size() < obs_num_limit&&
						i_obs_region_1.size() < obs_num_limit &&
						i_obs_region_2.size() < obs_num_limit &&
						i_obs_region_3.size() < obs_num_limit &&
						i_obs_region_4.size() < obs_num_limit){
				isFindObs = false;
				std::cout<<"GGGGGGGGGGG"<<std::endl;
			}
		}


	}
	bool alldir::WaitObsLeave()
	{
		std::cout<<"wait "<<std::endl;

		static bool isInitial = true;
		static bool isStop = false;
		static int Acceleration_count = 0;

		static int delay_count_1 = 0;
		static int delay_count_2 = 0;

		static float start_vx = 0;
		static float start_vy = 0;
		static float start_w = 0;
		static float decay_vx = 0;
		static float decay_vy = 0;

		float Acceleration_time = OBS_Stop_Sec; //s
		int Acceleration_count_limit = int(Acceleration_time/time_sample);


		float delay_time = OBS_Wait_Sec; //s
		int delay_count_limit = int(delay_time/time_sample);

		if(isInitial){
			start_vx = Rev_odom_vx;
			start_vy = Rev_odom_vy;
			decay_vx = -1*start_vx/Acceleration_count_limit;
			decay_vy = -1*start_vy/Acceleration_count_limit;
			isInitial = false;
			isStop = false;
		}
		else{

			if(!isStop){


				Acceleration_count += 1;

				if(Acceleration_count < Acceleration_count_limit){
					start_vx += decay_vx;
					start_vy += decay_vy;
				}
				else{
					start_vx = 0;
					start_vy = 0;
					isStop = true;
					Acceleration_count = 0;
				}

			}
			else{

				if(isFindObs || isAvoidObs){

					delay_count_2 = 0;

					if(delay_count_1 < delay_count_limit){
						delay_count_1 += 1;
					}
					else{
						isInitial = true;
						isStop = false;
						Acceleration_count = 0;
						delay_count_1 = 0;
						delay_count_2 = 0;


						start_vx = 0;
						start_vy = 0;
						start_w = 0;
						decay_vx = 0;
						decay_vy = 0;

						p_state_ = P_STATE_AVOID_REPLANNING;
						AvoidObs_state = P_OBS_REPLANNING;

						//如果是極限狀態 不避障 等待障礙物消失
						// if(OBS_limilMode)
						// {
						// 	p_state_ = P_STATE_AVOID_DEADLOCK;
						// 	AvoidObs_state = P_OBS_DEADLOCK;
						// 	ROS_INFO("OBS_limilMode DEAD");
						// }

						return true;
					}

				}
				else{//反正就是突然又不用閉障了

					delay_count_1 = 0;

					if(delay_count_2 < delay_count_limit){
						delay_count_2 += 1;
					}
					else{
						std::cout<<"no replanning"<<std::endl;
						isInitial = true;
						isStop = false;
						Acceleration_count = 0;
						delay_count_1 = 0;
						delay_count_2 = 0;

						start_vx = 0;
						start_vy = 0;
						start_w = 0;
						decay_vx = 0;
						decay_vy = 0;

						p_state_ = P_STATE_MOVE;
						AvoidObs_state = P_OBS_NORMAL;
						return true;
					}
				}

			}


			float Vx = start_vx;
			float Vy = start_vy;
			float W_rw = start_w;

			if(start_vx == 0 && start_vy == 0){
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;

			}
			else{

				Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
			}

			std::vector<unsigned char> command;


			sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
					rpm[1], theta[1],
					rpm[3], theta[3],
					rpm[2], theta[2],
					command
					);

			SendPackage(command);



		}

		return false;

	}
	void alldir::RePlanning(int &subpath_index,
			std::vector<Eigen::Vector3f> &i_obs_gap_0,
			std::vector<Eigen::Vector3f> &i_obs_gap_2,
			std::vector<Eigen::Vector3f> &i_obs_gap_4,
			std::vector<Eigen::Vector3f> &all_point)
	{


		static int target_ind = 0;
		static bool isInitial_rot = true;
		//float V_target = 0.2;//m/s
		////////////////////////////////////計算雷達直接鎖死秒數
		static int laser_dead_count = 0;
		int laser_dead_time = 10;
		int laser_dead_count_limit = int(laser_dead_time/time_sample); //100次
		/////////////////////////////////////////////////////

		float Vx = 0;
		float Vy = 0;
		float W_rw = 0;

		float cmd_velocity = 0.0;

		static int cmd_angular_cnt = 0;
		int cmd_angular_cnt_limit = 10;

		static float pre_angular_error = 0;

		float angular_kp = 0.5;
		float angular_kd = 0.1;
		float angular_ki = 0;

		float angular_error = 0;

		static bool isFInish = false;

		std::string kinematics_model;

		int id = A_misson[ready_path_index].sub_missonPath[now_path_index].start;
		for(int i=0;i<A_misson[ready_path_index].ALL_pathnode.size();i++){
			if(A_misson[ready_path_index].ALL_pathnode[i].id==id)
			{
				kinematics_model = A_misson[ready_path_index].ALL_pathnode[i].kin ;
				break;
			}
		}


		Eigen::Vector3f target_pos, now_pos;

		Eigen::Vector3f robot_pos = slam_pose_;

		int now_index = 0;


		if(isInitial_rot)
		{
			ROS_INFO("-------------------");
			ROS_INFO("isInitial_rot");
			if(kinematics_model == "diff")
			{
				obs_way_theta = 0;

				target_ind = calc_target_index(robot_pos, Rev_odom_v, A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint, now_index);
				target_pos = A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint[target_ind];
				now_pos = A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint[now_index];

				// float x_error_robot = 0.0 ;
				// float y_error_robot = 0.0 ;
				// tra.world2robot( target_pos,  robot_pos, x_error_robot, y_error_robot);

				// float way_theta = atan2(y_error_robot, x_error_robot);

				float cmd_angular_velocity = 0.0;
				float cmd_angular_velocity_buf = 0.0;
				float angular_error = 0.0;


				float goal_angle = atan2((target_pos.y() - now_pos.y()) , (target_pos.x() - now_pos.x()));

				angular_error = goal_angle - robot_pos.z();


				if(angular_error>M_PI)
					angular_error=angular_error-2*M_PI;
				else if(angular_error<-M_PI)
					angular_error=angular_error+2*M_PI;

				float angular_p_error = angular_error;
				float angular_d_error = angular_error - pre_angular_error;
				pre_angular_error = angular_error;

				cmd_angular_velocity = angular_kp*angular_p_error + angular_kd*angular_d_error;



				if(fabs(cmd_angular_velocity) > 0.3){
					cmd_angular_velocity_buf = (cmd_angular_velocity/fabs(cmd_angular_velocity))*0.3;
					cmd_angular_cnt += 1;
					if(cmd_angular_cnt > cmd_angular_cnt_limit)
					{
						cmd_angular_cnt = cmd_angular_cnt_limit;
						cmd_angular_velocity = cmd_angular_velocity_buf;
					}
					else
					{
						cmd_angular_velocity = cmd_angular_velocity_buf * cmd_angular_cnt /cmd_angular_cnt_limit;
					}

				}

				if(fabs(cmd_angular_velocity) < 0.05){
					cmd_angular_velocity = (cmd_angular_velocity/fabs(cmd_angular_velocity))*0.05;
				}

				if(fabs(angular_error) < 0.04){
					isFInish = true;
					cmd_angular_velocity = 0;
				}

				Vx = 0;
				Vy = 0;

				W_rw = cmd_angular_velocity;


				Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);
			}
			else
				isFInish = true;

			if(fabs(Rev_odom_V1 - 0) <= 0.0 && fabs(Rev_odom_V2 - 0) <= 0.02 &&
					fabs(Rev_odom_V3 - 0) <= 0.02 && fabs(Rev_odom_V4 - 0) <= 0.02)
			{
				laser_dead_count += 1;
				if(laser_dead_count > laser_dead_count_limit)
				{
					ROS_INFO("laser dead");
					p_state_ = P_STATE_AVOID_DEADLOCK;
					AvoidObs_state = P_OBS_DEADLOCK;
					ROS_INFO("OBS_limilMode DEAD");
					laser_dead_count = 0;
				}

			}

			if(isFInish){
				rpm[0] = 0;
				rpm[1] = 0;
				rpm[2] = 0;
				rpm[3] = 0;

				isInitial_rot = false;
				cmd_angular_cnt = 0;
				pre_angular_error = 0;
				isFInish = false;
				if(isFindObs)
				{
					std::cout<<"========================test"<<std::endl;
					isInitial_rot = true;
					isFindObs = false;

					if(OBS_limilMode)
					{
						p_state_ = P_STATE_AVOID_DEADLOCK;
						AvoidObs_state = P_OBS_DEADLOCK;
						ROS_INFO("OBS_limilMode DEAD");
					}
					else
					{
						p_state_ = P_STATE_AVOID_DEADLOCK;
						AvoidObs_state = P_OBS_DEADLOCK;
						ROS_INFO("OBS_limilMode not open");

						//
					}

				}
			}

			std::vector<unsigned char> command;

			sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
					rpm[1], theta[1],
					rpm[3], theta[3],
					rpm[2], theta[2],
					command
					);

			SendPackage(command);


		}
		else
		{

            isFindObs = false;
			OBS_limilMode = false;


			ROS_INFO("-------------------");
			ROS_INFO("RePlanning");

			//這是最終佔點，用來告知到底目前位置能不能避障
			target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size()-1];

			//找到最近得障礙物點並向左或右延伸的點，用來與目標點做新路徑
			Eigen::Vector3f Key_Point;

			float p_mobile_width = CarWidth;
			float p_mobile_length = CarLength;
			float dis_min = sqrt(p_mobile_width*p_mobile_width + p_mobile_length*p_mobile_length)/2.0 +1.0;

			float x_error = target_pos.x() - robot_pos.x();
			float y_error = target_pos.y() - robot_pos.y();
			float dis_error = sqrt(x_error*x_error + y_error*y_error);

			bool isWorkable = false;

			if(dis_error > dis_min){

				int now_index = 0;
				float odom_v = 0.8;

				//得到前視距離點
				int target_ind = calc_target_index_obs(robot_pos, odom_v, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);

				target_pos = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[target_ind];

				//如果是二次避障階段 要使用閉障之路徑再規劃
				if(obs_avoid_flag)
				{
					target_ind = calc_target_index_obs(robot_pos, Rev_odom_v, avoid_path, now_index);
					target_pos = avoid_path[target_ind];


				}

				isWorkable = FindKeyPoint(target_pos, Key_Point, i_obs_gap_0, i_obs_gap_2, i_obs_gap_4 ,all_point);

				if(isWorkable){

					//讓複製的路徑儘量是平移
					Eigen::Vector3f move_point;
					float key_dis = getDistance(Key_Point.x() , Key_Point.y(), robot_pos.x(), robot_pos.y());
					int CreateNewPath_int = 0;
					if(obs_avoid_flag)
					{
						CreateNewPath_int = calc_target_index_key (robot_pos, key_dis, avoid_path, now_index);
						move_point = avoid_path[CreateNewPath_int];
					}
					else
					{
						CreateNewPath_int = calc_target_index_key (robot_pos, key_dis, A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint, now_index);
						move_point =  A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[CreateNewPath_int];
					}



					CreateNewPath(move_point, Key_Point, subpath_index, avoid_path);

					ROS_INFO("----------------");
					//ROS_INFO("NEW target_pos: %f, %f", target_pos.x(), target_pos.y());
					ROS_INFO("NEW Key_Point: %f, %f", Key_Point.x(), Key_Point.y());


					p_state_ = P_STATE_AVOID_OBS;
					AvoidObs_state = P_OBS_AVOID;
					isblind = true;
					isAvoidObs = true;
					isInitial_rot = true;
					std::cout<<"==isblind==rep== "<<isblind<<std::endl;
				}
				else{
					//找不到key_point dead
					p_state_ = P_STATE_AVOID_DEADLOCK;
					AvoidObs_state = P_OBS_DEADLOCK;
					ROS_INFO("no find DEAD");
				}

			}
			else{
				//離站點太近了 會影響停車，dead
				p_state_ = P_STATE_AVOID_DEADLOCK;
				AvoidObs_state = P_OBS_DEADLOCK;
				ROS_INFO("too close station DEAD");
			}


			ROS_INFO("RePlanning OFF");
		}

	}

	bool alldir::FindKeyPoint(Eigen::Vector3f& target, Eigen::Vector3f& key_p,
			std::vector<Eigen::Vector3f> &i_obs_gap_0,
			std::vector<Eigen::Vector3f> &i_obs_gap_2,
			std::vector<Eigen::Vector3f> &i_obs_gap_4,
			std::vector<Eigen::Vector3f> &all_point)
	{

		std::cout<<"find"<<std::endl;
		//含雷達的長寬
		float p_mobile_width = CarWidth;
		float p_mobile_length = CarLength;
		float dis_min = sqrt(p_mobile_width*p_mobile_width + p_mobile_length*p_mobile_length)/2.0 ;

		//float scalar = 1.0;

		//基本淘汰完全不必算之部份
		//float gap_limit = p_mobile_width + 2*OBS_Side;
		float Sca = OBS_SideMiss_Scale *0.9 ;
					if(Sca <1.28)
						Sca = 1.28;

		float gap_limit = Car_Side*2.0 + 2.0*OBS_Side *Sca;


		std::vector<gap> gap_array;
		std::vector<Eigen::Vector3f> ok_key_point;//用來存所有可以通過的點
		Eigen::Vector3f robot_pos = slam_pose_;


		Eigen::Vector3f target_robot;

		Eigen::Vector3f _robot;
		_robot<<0.0 ,0.0 ,1.0;
		Eigen::Vector3f move_line;					//位移方線的直線（方程式）
		Eigen::Vector3f obs_line;					//障礙物所形成的一條線 （方程式）
		Eigen::Vector3f vertical_line;				//車子座標中 x的直線（前面）（方程式）
		std::vector<Eigen::Vector3f> obs_line_point;



		bool flag_right = false;
		bool flag_left = false;
		bool isWorkable = false;

		int obs_num_limit = OBS_isObs;

		float x=0.0;
		float y=0.0;

		tra.world2robot(target, robot_pos,  x,  y);
		target_robot.x() = x;
		target_robot.y() = y;
		target_robot.z() = 1.0;

		//位移方線的直線（方程式）
		move_line = _robot.cross(target_robot);

		// //判斷我要閉障時，本身左右有沒有障礙物
		//
		//左邊

		int count_dis =0;
		for(int i=0;i<i_obs_gap_0.size() ;i++)
		{
			if(i_obs_gap_0.size() >= obs_num_limit)
			{
				if(i_obs_gap_0[i].x()< target_robot.x()/2)
				{
					float obs_dis = fabs(move_line.x() *i_obs_gap_0[i].x() +move_line.y() *i_obs_gap_0[i].y() +move_line.z())/sqrt(move_line.x() * move_line.x() +move_line.y()*move_line.y());
					if(obs_dis < gap_limit/2.0*OBS_SideMiss_Scale)
					{
						count_dis++;
					}
				}
			}
		}
		std::cout<<"flag_left  "<<count_dis<<std::endl;
		if(count_dis > obs_num_limit)
		{
			flag_left = true;
		}

		//右邊
		count_dis =0;
		for(int i=0;i<i_obs_gap_4.size() ;i++)
		{
			if(i_obs_gap_4.size() >= obs_num_limit)
			{
				if(i_obs_gap_4[i].x()< target_robot.x()/2)
				{
					float obs_dis = fabs(move_line.x() * i_obs_gap_4[i].x() +move_line.y() *i_obs_gap_4[i].y() +move_line.z())/sqrt(move_line.x() * move_line.x() +move_line.y()*move_line.y());
					if(obs_dis < gap_limit/2.0*OBS_SideMiss_Scale)
					{
						count_dis++;

					}
				}
			}
		}
		std::cout<<"flag_right  "<<count_dis<<std::endl;
		if(count_dis > obs_num_limit)
		{
			flag_right = true;
		}
		count_dis = 0;


		int tan_min_i =0 ;

		//其中一邊無障礙物
		if(!flag_left || !flag_right)
		{
			//float obs_dis_limit=10000.0;

			//找離我閉障方向最靠近正前方的點
			float tan_min = 3.14;
			for(int i=0;i<i_obs_gap_2.size() ;i++)
			{
				if(i_obs_gap_2[i].x()> -1*p_mobile_length)
				{
					float tan = fabs(atan2(i_obs_gap_2[i].y(),i_obs_gap_2[i].x()));
					if(tan<tan_min)
					{
						tan_min = tan;
						tan_min_i=i;
					}
				}
			}
			std::cout<<"i_obs_gap_2 .size() "<<i_obs_gap_2.size()<<std::endl;
			std::cout<<"see 1  "<<std::endl;
			std::cout<<"tan_min_i 1  "<<tan_min_i<<std::endl;
			std::cout<<"tan_min 1  "<<tan_min<<std::endl;


			//障礙物線的計算
			if(i_obs_gap_2.size() > 0 )
			{
				i_obs_gap_2[tan_min_i].z() = 1.0;
				Eigen::Vector3f obs_i_obs_region_2;
				obs_i_obs_region_2<<i_obs_gap_2[tan_min_i].x() ,i_obs_gap_2[tan_min_i].y() + 0.1 ,1.0;
				obs_line = obs_i_obs_region_2.cross(i_obs_gap_2[tan_min_i]);//障礙物所形成的一條線 （方程式）
				std::cout<<"see 2  "<<std::endl;


			}
			else
				return false;


			float scale = sqrt(obs_line.x() * obs_line.x() + obs_line.y() * obs_line.y());
			Eigen::Vector3f normalize_obs_line_Line;
			normalize_obs_line_Line << obs_line.x()/scale,obs_line.y()/scale,obs_line.z()/scale;
			std::cout<<"see 3  "<<std::endl;

			//畫obsline的線
			std::vector<Eigen::Vector3f> drawline_;
			Eigen::Vector3f linepoint;
			linepoint << 0, 0, 0;

			float y = 8.0;
			float x = (-normalize_obs_line_Line.z() - normalize_obs_line_Line.y()*y)/normalize_obs_line_Line.x();
			linepoint << x, y, 1.0;
			drawline_.push_back(linepoint);
std::cout<<"see 4  "<<std::endl;

			x = (-normalize_obs_line_Line.z() + normalize_obs_line_Line.y()*y)/normalize_obs_line_Line.x();
			linepoint << x, -y, 1.0;
			drawline_.push_back(linepoint);

			for(int i = 0;i<drawline_.size();i++)
			{
				tra.robot2world(drawline_[i],robot_pos,obs_way_theta,x,y);
				drawline_[i].x() = x;
				drawline_[i].y() = y;

			}
std::cout<<"see 5  "<<std::endl;
			drawLine( obs_LineMarker, 0.0, 1.0, 0.0, drawline_);
			//畫obsline的線


			//找平行於車子的閉障線，把侷限於障礙區區域畫出來

			std::vector<Eigen::Vector3f> all_point_world;
			std::vector<Eigen::Vector3f> obs_line_point_world;

			for(int i=0;i<all_point.size() ;i++)
			{
				//if(all_point[i].x()> 0 && all_point[i].x() < all_obs_dis )
				{	all_point[i].z() = 1.0;
					//float obs_dis = fabs(obs_line.x() *all_point[i].x() +obs_line.y() *all_point[i].y() +obs_line.z())/sqrt(obs_line.x() * obs_line.x() +obs_line.y()*obs_line.y());
					float obs_dis = fabs(normalize_obs_line_Line.transpose() * all_point[i]);
					if(obs_dis < 1.0)
					{
						obs_line_point.push_back(all_point[i]); //取得所有的輪廓點
					}

				}
				//畫出軌跡再rviz
				// float all_point_buf_x = 0;
				// float all_point_buf_y = 0;
				// tra.robot2world(all_point[i],robot_pos,obs_way_theta,all_point_buf_x,all_point_buf_y);

				// Eigen::Vector3f p ;
				// p <<all_point_buf_x , all_point_buf_y, 1.0;
				// all_point_world.push_back(p);
			}

			// //垂直障礙物線的計算
			// Eigen::Vector3f vertical_p;
			// vertical_p <<i_obs_region_2[tan_min_i].x()+0.1 ,i_obs_region_2[tan_min_i].y() ,1.0;
			// vertical_line = vertical_p.cross(i_obs_region_2[tan_min_i]);

			Eigen::Vector3f vertical_p;
			vertical_p <<1.1 ,0.0 ,1.0;
			Eigen::Vector3f vertical_p1;
			vertical_p1 <<0.0 ,0.0 ,1.0;
			vertical_line = vertical_p.cross(vertical_p1);

			scale = sqrt(vertical_line.x() * vertical_line.x() + vertical_line.y() * vertical_line.y());
			Eigen::Vector3f normalize_vertical_line;
			normalize_vertical_line << vertical_line.x()/scale,vertical_line.y()/scale,vertical_line.z()/scale;



			int right_cnt = 0;
			int left_cnt = 0;

			for(int i = 0;i <obs_line_point.size() ;i++)
			{
				obs_line_point[i].z() = 1.0;
				float obs_dis = normalize_vertical_line.transpose() * obs_line_point[i];
				if(obs_dis > gap_limit/2.0)
					right_cnt +=1;
				else if(obs_dis < gap_limit/-2.0)
					left_cnt +=1;
			}

			//如果發現都無障礙物，會直製造無線遠的點讓他知道能通過
			std::cout<<"right_cnt  "<<right_cnt<<std::endl;
			std::cout<<"left_cnt  "<<left_cnt<<std::endl;
			if(right_cnt < 5)
			{
				Eigen::Vector3f obs_right;
				obs_right<<i_obs_gap_2[tan_min_i].x() ,i_obs_gap_2[tan_min_i].y()- 10.0 ,1.0;
				obs_line_point.push_back(obs_right);
			}
			if(left_cnt < 5)
			{
				Eigen::Vector3f obs_left;
				obs_left<<i_obs_gap_2[tan_min_i].x() ,i_obs_gap_2[tan_min_i].y()+ 10.0 ,1.0;
				obs_line_point.push_back(obs_left);

				std::cout<<"???  "<<left_cnt<<std::endl;
			}


			//畫vertical_line的線
			std::vector<Eigen::Vector3f> drawline_vertical;
			Eigen::Vector3f linepoint_vertical;
			linepoint_vertical << 0, 0, 0;

			x = 5.0;
			y = (-normalize_vertical_line.z()- x*normalize_vertical_line.x())/normalize_vertical_line.y();


			linepoint_vertical << x, y, 1.0;
			drawline_vertical.push_back(linepoint_vertical);


			y = (-normalize_vertical_line.z() + x*normalize_vertical_line.x())/normalize_vertical_line.y();
			linepoint_vertical << -x, y, 1.0;
			drawline_vertical.push_back(linepoint_vertical);


			for(int i = 0;i<drawline_vertical.size();i++)
			{
				tra.robot2world(drawline_vertical[i],robot_pos,obs_way_theta,x,y);
				drawline_vertical[i].x() = x;
				drawline_vertical[i].y() = y;

			}
			drawLine( vertical_lineMarker, 0.0, 0.0, 1.0, drawline_vertical);


			for(int i = 0;i <obs_line_point.size() ;i++)
			{
				float obs_line_point_buf_x = 0;
				float obs_line_point_buf_y = 0;
				tra.robot2world(obs_line_point[i],robot_pos,obs_way_theta,obs_line_point_buf_x,obs_line_point_buf_y);

				Eigen::Vector3f p ;
				p <<obs_line_point_buf_x , obs_line_point_buf_y, 1.0;
				obs_line_point_world.push_back(p);
			}
			//draw(ALLOBSPointMarker , 0.0, 0.0, 1.0, all_point_world);
			draw(obs_line_pointMarker , 1.0, 1.0, 0.0, obs_line_point_world);
			//畫出軌跡再rviz


			//排序
			if(obs_line_point.size() > 1 )
				for(int i=0;i<obs_line_point.size() ;i++)
				{
					for(int j=i+1;j<obs_line_point.size() ;j++)
					{
						if(obs_line_point[i].y() < obs_line_point[j].y())
						{
							Eigen::Vector3f buf;
							buf = obs_line_point[i];
							obs_line_point[i] = obs_line_point[j];
							obs_line_point[j] = buf;
							//std::cout<<"obs_line_point=============="<<std::endl;
						}
					}
				}

			std::cout<<"====obs_line_point====  "<<obs_line_point[0] <<std::endl;
			std::cout<<"====obs_line_point====  "<<obs_line_point[1] <<std::endl;

			std::cout<<"====obs_line_point.size()====  "<<obs_line_point.size() <<std::endl;




			//找點
			float obs_dis=0.0;
			if(obs_line_point.size() > 1)
				for(int i=0;i<obs_line_point.size();i++)
				{
					obs_dis = obs_line_point[i].y() - obs_line_point[i+1].y();
					//std::cout<<"====obs_dis====  "<<obs_dis <<std::endl;

					if(obs_dis >= gap_limit)
					{
						gap  gap_sort;

						gap_sort.distance = obs_dis;
						gap_sort.p1 = obs_line_point[i];
						gap_sort.p2 = obs_line_point[i+1];

						gap_array.push_back(gap_sort);
					}

				}


		}

		ROS_INFO("--------------");
		ROS_INFO("FindKeyPoint");
		for(int i=0; i<gap_array.size(); i++)
			ROS_INFO("gap_array: %f", gap_array[i].distance);
		ROS_INFO("gap_limit: %f", gap_limit);

		//找最大gap
		if(gap_array.size() > 0){
			Eigen::Vector3f obs_pos;

			int temp_i;
			Eigen::Vector3f temp_vec;

			//排序
			for(int i=0;i<gap_array.size();i++)
			{
				for(int j=i+1;j<gap_array.size();j++)
				{
					if(gap_array[i].distance < gap_array[j].distance)
					{
						temp_i = gap_array[i].distance;
						gap_array[i].distance = gap_array[j].distance;
						gap_array[j].distance = temp_i;
						temp_vec = gap_array[i].p1;
						gap_array[i].p1 = gap_array[j].p1;
						gap_array[j].p1 = temp_vec;
						temp_vec = gap_array[i].p2;
						gap_array[i].p2 = gap_array[j].p2;
						gap_array[j].p2 = temp_vec;
					}
				}
			}



			for(int i = 0;i <gap_array.size();i++)
			{
				float max_dis = gap_array[i].distance;
				ROS_INFO("max_dis: %f", max_dis);

				//gap比我認為的大 視為可過
				if(max_dis >= gap_limit){
					Eigen::Vector3f p0 = gap_array[i].p1;
					Eigen::Vector3f p1 = gap_array[i].p2;
					Eigen::Vector3f p2 = Eigen::Vector3f( (p0.x()+p1.x())/2,  (p0.y()+p1.y())/2, 0.0);


					//判斷左邊右邊（這裡存的座標都是相對於車中心（因為雷射來的））
					// float region_theta = atan2(p2.y(), p2.x());
					// if(region_theta >= 0) avoid_way = 1;
					// else avoid_way = -1;


					if(p2.y() >= 0) avoid_way = 1;
					else avoid_way = -1;

					Eigen::Vector3f local_key_p = Eigen::Vector3f(0.0, 0.0, 0.0);
					float dx = 0.0 ,dy = 0.0;

					//計算要往哪邊位移 （左右）dx都為0

					if(avoid_way > 0){
						dy = gap_limit/2.0;
						ROS_INFO("--------left--------");
						if(flag_left)
						{
							max_dis=0;
							ROS_INFO("------GG-----");
						}
						//找我尋找障礙物的末端點是哪0
						float p0_robot = p0.y();
						float p1_robot = p1.y();
						if(p0_robot > 0 && p1_robot > 0)
						{
							if(p0_robot > p1_robot)
							{
								obs_pos = p1;
							}
							else{
								obs_pos = p0;
							}
						}
						else{
							if(p0_robot < 0)
								obs_pos = p0;
							else
								obs_pos = p1;
						}
					}
					else{
						dy = -1.0 * gap_limit/2.0;
						ROS_INFO("--------right--------");
						if(flag_right)
						{
							max_dis=0;
						}
						//找我尋找障礙物的末端點是哪0
						float p0_robot = p0.y();
						float p1_robot = p1.y();
						if(p0_robot < 0 && p1_robot < 0)
						{
							if(p0_robot > p1_robot)
							{
								obs_pos = p0;
							}
							else{
								obs_pos = p1;
							}
						}
						else{
							if(p0_robot > 0)
								obs_pos = p0;
							else
								obs_pos = p1;
						}
					}

					//末端點加上一半車身距離
					local_key_p[0] = obs_pos.x();
					local_key_p[1] = obs_pos.y() + dy;

					// ROS_INFO("local_key_p: %f, %f", local_key_p.x(), local_key_p.y());


					//變成世界座標
					float obs_pos_world_x=0.0, obs_pos_world_y=0.0;
					tra.robot2world( local_key_p,  robot_pos,  obs_way_theta, dx, dy);
					tra.robot2world( obs_pos,  robot_pos,  obs_way_theta, obs_pos_world_x, obs_pos_world_y);

					//key_p = Eigen::Vector3f(dx, dy, robot_pos.z()  + obs_way_theta);
					key_p = Eigen::Vector3f(dx, dy, 0.0);
					ok_key_point.push_back(key_p);

					//std::cout<<"==========  "  <<std::endl;
					std::cout<<"local_key_p  " <<local_key_p <<std::endl;
					//std::cout<<"key_p  " <<key_p <<std::endl;
					//std::cout<<"robot_pos  " <<robot_pos <<std::endl;
					//std::cout<<"==========  "  <<std::endl;

					//剛剛gap用最小來算，當我們確定位移方向後，判斷車身姿態改變gap_limit大小

					// float x_error_world = key_p.x() - obs_pos_world_x;
					// float y_error_world = key_p.y() - obs_pos_world_y;

					// float way_theta = atan2(y_error_world, x_error_world);

					// float warn_theta = (way_theta - robot_pos.z() + 2*M_PI) ;

					// if(warn_theta > (2*M_PI)) warn_theta = warn_theta - 2*M_PI;
					// if(warn_theta > M_PI) warn_theta = 2*M_PI - warn_theta;
					// if(warn_theta>M_PI/2)
					// 	warn_theta=M_PI-warn_theta;



					// if(max_dis >= gap_limit)
					// {
					// 	isWorkable = true;
					// 	break;
					// }


					// ROS_INFO("gap_limit: %f", gap_limit);
					// if(max_dis >= gap_limit)
					// {
					// 	//重新計算新的key_p點 因為要避障的長度改變

					// 	//計算要往哪邊位移 （左右）dx都為0
					// 	if(avoid_way > 0){
					// 		dx = (gap_limit/2.0)*cos(M_PI/2);
					// 		dy = (gap_limit/2.0)*sin(M_PI/2);
					// 		ROS_INFO("--------left--------");
					// 	}
					// 	else{
					// 		dx = (gap_limit/2.0)*cos(-1*M_PI/2);
					// 		dy = (gap_limit/2.0)*sin(-1*M_PI/2);
					// 		ROS_INFO("--------right--------");
					// 	}

					// 	//末端點加上一半車身距離
					// 	local_key_p[0] = obs_pos.x() + dx;
					// 	local_key_p[1] = obs_pos.y() + dy;

					// 	//std::cout<<"local_key_p  "<<local_key_p<<std::endl;
					// 	//變成世界座標
					// 	tra.robot2world( local_key_p,  robot_pos,  obs_way_theta, dx, dy);

					// 	key_p = Eigen::Vector3f( dx, dy, 0.0);

					// 	//std::cout<<"key_p  "<<key_p<<std::endl;

					// 	isWorkable = true;
					// 	break;
					// }
					//=============change================

				}
			}
			Eigen::Vector3f target_pos;
			int min_distance = 10000000;
			int min_i = 0;
			target_pos = A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint[A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint.size()-1];
			for(int i=0;i<ok_key_point.size();i++)//尋找所有的key_point看哪個碘離目標最近
			{
					float key_point_distance = getDistance(ok_key_point[i].x(), ok_key_point[i].y(), target_pos.x(), target_pos.y());
					if(key_point_distance < min_distance)
					{
							min_distance = key_point_distance;
							min_i = i;
					}
			}
			key_p = ok_key_point[min_i];
			isWorkable = true;
			std::cout<<"==========  "  <<std::endl;
			std::cout<<"key_p  " <<key_p <<std::endl;
			std::cout<<"robot_pos  " <<robot_pos <<std::endl;
			std::cout<<"==========  "  <<std::endl;


		}

    ok_key_point.clear();
		obs_line_point.clear();
		gap_array.clear();



		return isWorkable;


	}
	void alldir::CreateNewPath(Eigen::Vector3f& target, Eigen::Vector3f& key_point, int subpath_index, std::vector<Eigen::Vector3f> &new_path)
	{

		float x_error = key_point.x() - target.x();
		float y_error = key_point.y() - target.y();


		// std::fstream fout;
		//     fout.open("avoid_LINE",std::fstream::out);
		if(obs_avoid_flag)
		{
			std::vector<Eigen::Vector3f> path;
			for(int i=0;i<avoid_path.size();i++){
				Eigen::Vector3f path_buf;
				path_buf.x() = avoid_path[i].x() + x_error;
				path_buf.y() = avoid_path[i].y() + y_error;
				path_buf.z() = 1.0;

				path.push_back(path_buf);
			}
			avoid_path.clear();
			new_path = path;


			//測試中
			//畫出軌跡再rviz
			draw(AvoidLineMarker , 1.0, 0.1, 0.1, avoid_path);
			//畫出軌跡再rviz


		}
		else{
			avoid_path.clear();
			std::vector<Eigen::Vector3f> path;

			for(int i=0;i<A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint.size();i++){
				Eigen::Vector3f path_buf;
				path_buf.x() = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[i].x() + x_error;
				path_buf.y() = A_misson[ready_path_index].sub_missonPath[subpath_index].sub_missonPath_subPoint[i].y() + y_error;
				path_buf.z() = 1.0;

				path.push_back(path_buf);

			}
			new_path = path;

			draw(AvoidLineMarker , 1.0, 0.1, 0.1, new_path);
		}




	}
	bool alldir::AvoidObs()
	{std::cout<<"=========AvoidObs=======  " <<std::endl;


		//-----------------------------------
		static int target_ind = 0;
		static bool isInitial_rot = true;
		static bool isInitial_move = true;
		float V_target = 0.2;//m/s


		float Vx = 0;
		float Vy = 0;
		float W_rw = 0;


		float cmd_velocity = 0.0;



		Eigen::Vector3f robot_pos, target_pos;
		int now_index = 0;
		robot_pos = slam_pose_;



		//這是最終佔點，用來告知到底目前位置能不能避障
			target_pos = A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint[A_misson[ready_path_index].sub_missonPath[now_path_index].sub_missonPath_subPoint.size()-1];

			//找到最近得障礙物點並向左或右延伸的點，用來與目標點做新路徑
			Eigen::Vector3f Key_Point;

			float p_mobile_width = CarWidth;
			float p_mobile_length = CarLength;
			float dis_min = sqrt(p_mobile_width*p_mobile_width + p_mobile_length*p_mobile_length)/2.0 +1.0;

			float x_error = target_pos.x() - robot_pos.x();
			float y_error = target_pos.y() - robot_pos.y();
			float dis_error = sqrt(x_error*x_error + y_error*y_error);


			int id = A_misson[ready_path_index].sub_missonPath[now_path_index].end;
			std::cout<<"id  "<<id<<std::endl;
			std::cout<<"dis_min  "<<dis_min<<std::endl;
			std::cout<<"dis_error  "<<dis_error<<std::endl;

			if(dis_error < dis_min){
				if(isFindObs == false)
				{
					target_ind = 0;
					isInitial_move = true;
					obs_avoid_flag =false;

					std::vector<unsigned char> command;


					sendreceive.Package_AllDir_encoder(0, theta[0],
							0, theta[1],
							0, theta[3],
							0, theta[2],
							command);


					SendPackage(command);

					p_state_ = P_STATE_MOVE;
					AvoidObs_state = P_OBS_NORMAL;
					obs_return = true;


			//測試中
			//畫出軌跡再rviz
			std::vector<Eigen::Vector3f> NULL_buf;
			draw(AvoidLineMarker , 1.0, 0.0, 0.0, NULL_buf);
				}
				else
				{
					target_ind = 0;
					isInitial_move = true;
					obs_avoid_flag =true;
					//離站點太近了 會影響停車，dead
					p_state_ = P_STATE_AVOID_DEADLOCK;
					AvoidObs_state = P_OBS_DEADLOCK;
					ROS_INFO("too close station DEAD");
				}
				return false;


			}






		if(isAvoidObs == true && isFindObs == false){

			robot_pos = slam_pose_;

			// std::fstream fout;
			// fout.open("obs_robot_pos",std::fstream::out);
			// fout<<robot_pos.x() <<"\t" <<robot_pos.y() <<std::endl;

			int now_index;


			target_ind = calc_target_index_obs(robot_pos, Rev_odom_v, avoid_path, now_index);


			target_pos = avoid_path[target_ind];
			std::cout<<"avoid_path.size()  "<<avoid_path.size() <<std::endl;



			Eigen::Vector3f now_pos = avoid_path[now_index];



			float x_error_robot = 0.0 ;
			float y_error_robot = 0.0 ;
			tra.world2robot( target_pos,  robot_pos, x_error_robot, y_error_robot);


			float way_theta = atan2(y_error_robot, x_error_robot);
			cmd_velocity = V_target;

			// std::cout<<"target_ind  "<<target_ind<<std::endl;
			// std::cout<<"target_pos  "<<target_pos<<std::endl;
			// std::cout<<"now_index  "<<now_index<<std::endl;
			// std::cout<<"now_pos  "<<now_pos<<std::endl;
			// std::cout<<"robot_pos  "<<robot_pos<<std::endl;

			// std::cout<<"OBS move way_theta  "<<way_theta<<std::endl;



			Vx = cmd_velocity*cos(way_theta);
			Vy = cmd_velocity*sin(way_theta);
			W_rw = 0.0;


			static float time_count_avoid = 1;
			float th_err = way_theta - Rev_odom_t1;


			//避障移動到avoid的路徑時會致盲
			if(isInitial_move){
				float m_error = 0.0;
				tra.closeline( now_pos, target_pos, robot_pos, m_error);


				float th_time = OBS_Theta_Sec;
				int cnt_limit = int(th_time/time_sample);
				float th = way_theta;
				float way_theta_stime = 0;



				//移動出去時仿人行之t與th
				if(time_count_avoid < cnt_limit){
					way_theta_stime =  Rev_odom_t1 + (th_err/cnt_limit)*time_count_avoid;
					time_count_avoid += 1;

					if(fabs(th_err) < 0.03)
					{
						way_theta_stime = way_theta;
						time_count_avoid = cnt_limit;

					}

				}
				else{
					way_theta_stime = way_theta;
				}
				std::cout<<"OBS move way_theta_stime  "<<way_theta_stime<<std::endl;

				Vx = cmd_velocity*cos(way_theta_stime);
				Vy = cmd_velocity*sin(way_theta_stime);


				if(fabs(m_error) < 0.03 )
				{

					isblind = false;
					isInitial_move = false;
					time_count_avoid = 1;
				}
			}

			Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);


			if(isInitial_move){
				if(fabs(Rev_odom_t1 - theta[0]) <= 0.5) {

					std::vector<unsigned char> command;

					sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
							rpm[1], theta[1],
							rpm[3], theta[3],
							rpm[2], theta[2],
							command
							);

					SendPackage(command);
				}
				else
				{

					std::vector<unsigned char> command;

					sendreceive.Package_AllDir_encoder(0, theta[0],
							0, theta[1],
							0, theta[3],
							0, theta[2],
							command);


					SendPackage(command);

				}
			}

			std::vector<unsigned char> command;

			sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
					rpm[1], theta[1],
					rpm[3], theta[3],
					rpm[2], theta[2],
					command
					);

			SendPackage(command);





		}
		else if(isAvoidObs == false && isFindObs == true){
			target_ind = 0;
			isInitial_move = true;
			obs_avoid_flag =true;


			p_state_ = P_STATE_AVOID_REPLANNING;
			AvoidObs_state = P_OBS_REPLANNING;



		}
		else if(isAvoidObs == false && isFindObs == false){
			target_ind = 0;
			isInitial_move = true;
			obs_avoid_flag =false;

			std::vector<unsigned char> command;


			sendreceive.Package_AllDir_encoder(0, theta[0],
					0, theta[1],
					0, theta[3],
					0, theta[2],
					command);


			SendPackage(command);

			p_state_ = P_STATE_MOVE;
			AvoidObs_state = P_OBS_NORMAL;
			obs_return = true;


			//測試中
			//畫出軌跡再rviz
			std::vector<Eigen::Vector3f> NULL_buf;
			draw(AvoidLineMarker , 1.0, 0.0, 0.0, NULL_buf);
			//畫出軌跡再rviz
		}


		return false;


	}

	bool alldir::WaitDeadlock()
	{
		std::cout<<"==========  Deadlock  ======"<<std::endl;
		static int cnt = 0;
		float delay_time = 2.0;
		int cnt_limit = int(delay_time/time_sample);


		static int cnt_re = 0;
		float delay_time_re = 2.0;
		int cnt_limit_re = int(delay_time_re/time_sample);


		static int cnt_error = 0;

		//如果再死區狀態障礙物排除時
		if(!isFindObs){
			cnt += 1;

			if(cnt >= cnt_limit){
				cnt = 0;
				p_state_ = P_STATE_MOVE;
				AvoidObs_state = P_OBS_NORMAL;

				cnt_error = 0;

				return true;
			}
		}
		//障礙物無法完全排除 但能清出可行之閉障路徑
		else{
			cnt = 0;
			cnt_re +=1;
			if(cnt_re >= cnt_limit_re)
			{	std::cout<<"cnt_re "<< cnt_re<<std::endl;
				p_state_ = P_STATE_AVOID_REPLANNING;
				AvoidObs_state = P_OBS_REPLANNING;
				cnt_re = 0;
				cnt_error +=1;

				//請上層重新派任                   ////發佈error給上層需要人工排除障礙
				if(cnt_error > 10)
				{
					std_msgs::Int8 msg;

					msg.data = 0;

					ReMissionState_Publisher_.publish(msg);
				}


			}
		}

		// SendPackage(0, theta[0],
		// 		0, theta[1],
		// 		0, theta[3],
		// 		0, theta[2]);

		std::vector<unsigned char> command;

		sendreceive.Package_AllDir_encoder(0, theta[0],
				0, theta[1],
				0, theta[3],
				0, theta[2],
				command);


		SendPackage(command);

		return false;
	}
	void alldir::joystickCallback(const move_robot::joystick& joystick)
	{


		if(PUSE_BUTTON_RB != joystick.btn_id && PUSE_BUTTON_START!= joystick.btn_id)
			btn_id = joystick.btn_id;
		float vx = joystick.x * JOYSTICK_SCALAR;
		float vy = joystick.y * JOYSTICK_SCALAR;

		joystick_v = sqrt(pow(vy, 2) + pow(vx, 2));

		// if(joystick_v > 0.05)
        joystick_theta = atan2(vy, vx);
		// else
		// {
		// 	joystick_theta = 0;
		// 	joystick_v = 0;
		// }

		// if(joystick_theta <= -M_PI)
		// 	joystick_theta += (2*M_PI);

		isReveice_joystick = true;
		//std::cout<<"isReveice_joystick   " <<isReveice_joystick<<std::endl;
		//std::cout<< "joystick_theta  "<<joystick_theta *180/M_PI<<std::endl;



	}
	void alldir::joystick_move()
	{//==============change=============
		//std::cout<<"btn_id  "<<btn_id<<std::endl;

		if(btn_id == PUSE_BUTTON_A)
		{

			float us = joystick_theta;

			if(us>M_PI/2)
			{   us-=M_PI;
			}
			else if(us<-M_PI/2)
			{   us+=M_PI;
			}


			//L is the distance of both the wheel
			float L = LRWheeldis; //meter

			//W_rw
			float W_rw = (sin(us)/L)*joystick_v;


			if(fabs(W_rw) > 0.35){
				if(W_rw > 0)
					W_rw = 0.35;
				else
					W_rw = -0.35;
			}

			//==============change=============

			float V_avg = joystick_v;

			if(V_avg*cos(joystick_theta)<0)
			{
				V_avg = -1*V_avg;
			}




			float Vx = V_avg;
			float Vy = 0;

			//std::cout<<"Vx "<<Vx<<std::endl;

			Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

			std::vector<unsigned char> command;

			sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
					rpm[1], theta[1],
					rpm[3], theta[3],
					rpm[2], theta[2],
					command
					);
			std::cout<<"rpm "<<rpm[0]<<std::endl;


			SendPackage(command);

			ROS_INFO("============diff================");



		}
		else if(btn_id == PUSE_BUTTON_X)
		{ROS_INFO("turn_left");

			Car.four_wheel_Kinematics_rpm(0,0,0.35,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

			// SendPackage(rpm[0], theta[0],
			// 		rpm[1], theta[1],
			// 		rpm[3], theta[3],
			// 		rpm[2], theta[2]);

			std::vector<unsigned char> command;


			std::cout<<"Rev_odom_t1  "<< Rev_odom_t1<<std::endl;
			std::cout<<"theta[0]  "<< theta[0]/10*M_PI/180<<std::endl;
			std::cout<<"err  "<<Rev_odom_t1 - theta[0]/10*M_PI/180<<std::endl;

			if(fabs(Rev_odom_t1 - theta[0]/10*M_PI/180) <= 0.08 && fabs(Rev_odom_t2 - theta[1]/10*M_PI/180) <= 0.08 &&
					fabs(Rev_odom_t3 - theta[2]/10*M_PI/180) <= 0.08 && fabs(Rev_odom_t4 - theta[3]/10*M_PI/180) <= 0.08)
			{
				sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
						rpm[1], theta[1],
						rpm[3], theta[3],
						rpm[2], theta[2],
						command
						);
			}
			else
			{
				sendreceive.Package_AllDir_encoder(0, theta[0],
						0, theta[1],
						0, theta[3],
						0, theta[2],
						command
						);
			}

			SendPackage(command);

		}
		else if(btn_id == PUSE_BUTTON_Y)
		{ROS_INFO("turn_right");
			Car.four_wheel_Kinematics_rpm(0,0,-0.35,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

			// SendPackage(rpm[0], theta[0],
			// 		rpm[1], theta[1],
			// 		rpm[3], theta[3],
			// 		rpm[2], theta[2]);
			std::vector<unsigned char> command;

			if(fabs(Rev_odom_t1 - theta[0]/10*M_PI/180) <= 0.08 && fabs(Rev_odom_t2 - theta[1]/10*M_PI/180) <= 0.08 &&
					fabs(Rev_odom_t3 - theta[2]/10*M_PI/180) <= 0.08 && fabs(Rev_odom_t4 - theta[3]/10*M_PI/180) <= 0.08)
			{
				sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
						rpm[1], theta[1],
						rpm[3], theta[3],
						rpm[2], theta[2],
						command
						);
			}
			else
			{
				sendreceive.Package_AllDir_encoder(0, theta[0],
						0, theta[1],
						0, theta[3],
						0, theta[2],
						command
						);
			}

			SendPackage(command);

		}
		else if(btn_id == PUSE_BUTTON_BACK)
		{
			ROS_INFO("Wheel return");
			std::vector<unsigned char> command;
			sendreceive.Package_AllDir_encoder(0, 0,
						0, 0,
						0, 0,
						0, 0,
						command
						);
			SendPackage(command);
		}


	}
	void alldir::joystick_omni_move()
	{


		float V_avg = joystick_v;

		float th = joystick_theta ;

		if(fabs(V_avg) < 0.05)
			V_avg = 0;

		float Vx = V_avg *cos(th);
		float Vy = V_avg *sin(th);

		if(Vy==-0) Vy=0;
		if(Vx==-0) Vx=0;

		float W_rw =0.0;

		Car.four_wheel_Kinematics_rpm(Vx,Vy,W_rw,Rev_odom_t1,Rev_odom_t2,Rev_odom_t3,Rev_odom_t4,rpm,theta);

		std::vector<unsigned char> command;

		sendreceive.Package_AllDir_encoder(rpm[0], theta[0],
				rpm[1], theta[1],
				rpm[3], theta[3],
				rpm[2], theta[2],
				command
				);

		SendPackage(command);


		ROS_INFO("============omni================");



	}
	void alldir::Calculate_odom()
	{
		float a = 0.32;
		float b = 0.22;

		float x_w1 = a;
		float y_w1 = b;

		float x_w2 = -1*a;
		float y_w2 = b;

		float x_w3 = -1*a;
		float y_w3 = -1*b;

		float x_w4 = a;
		float y_w4 = -1*b;


		Rev_odom_V1 = (Rev_FL_rpm/(60.0*22.5))*2*M_PI*(0.15/2.0);
		Rev_odom_V2 = (Rev_RL_rpm/(60.0*22.5))*2*M_PI*(0.15/2.0);
		Rev_odom_V3 = (Rev_RR_rpm/(60.0*22.5))*2*M_PI*(0.15/2.0);
		Rev_odom_V4 = (Rev_FR_rpm/(60.0*22.5))*2*M_PI*(0.15/2.0);

		Rev_odom_t1 = Rev_FL_deg*M_PI/180.0;
		Rev_odom_t2 = Rev_RL_deg*M_PI/180.0;
		Rev_odom_t3 = Rev_RR_deg*M_PI/180.0;
		Rev_odom_t4 = Rev_FR_deg*M_PI/180.0;

		NormalizeAngle(Rev_odom_t1);
		NormalizeAngle(Rev_odom_t2);
		NormalizeAngle(Rev_odom_t3);
		NormalizeAngle(Rev_odom_t4);


		Rev_odom_vx = (cos(Rev_odom_t1)*Rev_odom_V1+cos(Rev_odom_t2)*Rev_odom_V2+cos(Rev_odom_t3)*Rev_odom_V3+cos(Rev_odom_t4)*Rev_odom_V4)/4;
		Rev_odom_vy = (sin(Rev_odom_t1)*Rev_odom_V1+sin(Rev_odom_t2)*Rev_odom_V2+sin(Rev_odom_t3)*Rev_odom_V3+sin(Rev_odom_t4)*Rev_odom_V4)/4;
		Rev_odom_v = sqrt(Rev_odom_vx*Rev_odom_vx + Rev_odom_vy*Rev_odom_vy);


		float W1 = (-y_w1*cos(Rev_odom_t1) + x_w1*sin(Rev_odom_t1))/(4*x_w1*x_w1 + 4*y_w1*y_w1);
		float W2 = (-y_w2*cos(Rev_odom_t2) + x_w2*sin(Rev_odom_t2))/(4*x_w2*x_w2 + 4*y_w2*y_w2);
		float W3 = (-y_w3*cos(Rev_odom_t3) + x_w3*sin(Rev_odom_t3))/(4*x_w3*x_w3 + 4*y_w3*y_w3);
		float W4 = (-y_w4*cos(Rev_odom_t4) + x_w4*sin(Rev_odom_t4))/(4*x_w4*x_w4 + 4*y_w4*y_w4);


		Rev_odom_w = W1*Rev_odom_V1 + W2*Rev_odom_V2 + W3*Rev_odom_V3 + W4*Rev_odom_V4;


		geometry_msgs::PoseStamped odom_odometry;
		odom_odometry.header.frame_id = "base_link";
		odom_odometry.pose.position.x = Rev_odom_vx;
		odom_odometry.pose.position.y = Rev_odom_vy;
		odom_odometry.pose.orientation.w = Rev_odom_w;
		odom_odometry.pose.orientation.z = 0;
		odometry_Publisher_.publish(odom_odometry);


		//std::cout<< "Rev_odom_t1  " <<Rev_odom_t1<<std::endl;

	}

	void alldir::RevProcess(double receive_period)
	{
		int Receive_Package_Size = 256;

		ros::Rate r_receive(1.0*30.0 / receive_period);
		std::vector<unsigned char> rev_buf;

		while(1){

			if(mySerial.serial_ok == true){

				unsigned char buff[Receive_Package_Size];

				int readByte = 0;

				readByte = read(mySerial.fd,buff,1);

				if(readByte > 0){
					for(int i=0; i<readByte; i++)
						rev_buf.push_back(buff[i]);

					if(rev_buf[0] != 0x0A)
					{
						rev_buf.clear();
						//std::cout<<"=========LOSS encoder========="<<std::endl;
					}
				}

				if(rev_buf.size() == 30)
				{
					sendreceive.RevProcess_AllDir_encoder(rev_buf, Rev_FR_rpm, Rev_FL_rpm, Rev_RR_rpm, Rev_RL_rpm, Rev_FR_deg, Rev_FL_deg, Rev_RR_deg, Rev_RL_deg);
					rev_buf.clear();
				}

			}

			r_receive.sleep();

		}

	}


	void alldir::RevProcess_Battery(double receive_period)
	{
		int Receive_Package_Size = 256;

		ros::Rate r_receive(1.0 / receive_period);

		while(1){

			if(mySerial.serial_ok == true){

				unsigned char buff[Receive_Package_Size];

				int readByte = 0;

				readByte = read(mySerial.fd,buff,Receive_Package_Size);

				std::vector<unsigned char> rev_buf;

				if(readByte > 0){
					for(int i=0; i<readByte; i++)
						rev_buf.push_back(buff[i]);
				}

				sendreceive.RevProcess_AllDir_encoder(rev_buf, Rev_FR_rpm, Rev_FL_rpm, Rev_RR_rpm, Rev_RL_rpm, Rev_FR_deg, Rev_FL_deg, Rev_RR_deg, Rev_RL_deg);

			}

			r_receive.sleep();

		}

	}
	void alldir::Stop()
	{
		static bool isInitial = true;
		static bool isStop = false;
		static int Acceleration_count = 0;


		static float start_vx = 0;
		static float start_vy = 0;
		static float decay_vx = 0;
		static float decay_vy = 0;

		float Acceleration_time = OBS_Stop_Sec; //s
		int Acceleration_count_limit = int(Acceleration_time/time_sample);



		if(isInitial){
			start_vx = Rev_odom_vx;
			start_vy = Rev_odom_vy;
			decay_vx = -1*start_vx/Acceleration_count_limit;
			decay_vy = -1*start_vy/Acceleration_count_limit;
			isInitial = false;
			if(Rev_odom_vx >0.1|| Rev_odom_vy >0.1)
				isStop = false;
			else isStop = true;
		}
		else{

			if(!isStop){


				Acceleration_count += 1;

				if(Acceleration_count < Acceleration_count_limit){
					start_vx += decay_vx;
					start_vy += decay_vy;

				}
				else{
					start_vx = 0;
					start_vy = 0;
					isStop = true;
					Acceleration_count = 0;
				}

				std::vector<unsigned char> command;


				sendreceive.Package_AllDir_encoder(0, theta[0],
						0, theta[1],
						0, theta[3],
						0, theta[2],
						command);

				SendPackage(command);

			}
			else{

				std::vector<unsigned char> command;

				sendreceive.Package_AllDir_encoder(0, theta[0],
						0, theta[1],
						0, theta[3],
						0, theta[2],
						command
						);

				SendPackage(command);
			}
		}

	}

	void alldir::Caculate_W_rw(float stop_angle, Eigen::Vector3f robot_pos, float &angular_error, float &pre_angular_error, float &cmd_angular_velocity, bool special)
	{
			float angular_kp = tracking_kp;
			float angular_kd = tracking_kd;
			float compare_angular_error = 1;
			float compare_stop_angle = 1;

			if(!special)
			{
					angular_error = stop_angle - robot_pos.z();
					if(fabs(angular_error) > M_PI){
						if(angular_error > 0) angular_error = angular_error - 2*M_PI ;
						else                  angular_error = angular_error + 2*M_PI;
					}


					float angular_p_error = angular_error;
					float angular_d_error = angular_error - pre_angular_error;
					pre_angular_error = angular_error;
					cmd_angular_velocity = angular_kp*angular_p_error + angular_kd*angular_d_error;
			}
			else
			{

					angular_error = stop_angle - robot_pos.z();
					if(stop_angle > 0)
					{
						compare_stop_angle = stop_angle - M_PI;
					}
					else
					{
						compare_stop_angle = stop_angle + M_PI;
					}

					std::cout<<"======================robot_pos.z() "<<robot_pos.z()<<std::endl;
					std::cout<<"======================stop_angle "<<stop_angle<<std::endl;
					std::cout<<"======================compare_stop_angle  "<< compare_stop_angle <<std::endl;
					compare_angular_error = compare_stop_angle - robot_pos.z();
					if(fabs(angular_error) > M_PI)
					{
						if(angular_error > 0) angular_error = angular_error - 2*M_PI ;
						else                  angular_error = angular_error + 2*M_PI;
					}
					if(fabs(compare_angular_error) > M_PI)
					{
						if(compare_angular_error > 0) compare_angular_error = compare_angular_error - 2*M_PI ;
						else                          compare_angular_error = compare_angular_error + 2*M_PI;
					}
					if(fabs(compare_angular_error) < fabs(angular_error))
					{
							angular_error = compare_angular_error;
					}
					else
					{
							angular_error = angular_error;
					}
					std::cout<<"======================angular_error  "<<angular_error<<std::endl;
					float angular_p_error = angular_error;
					float angular_d_error = angular_error - pre_angular_error;
					pre_angular_error = angular_error;
					cmd_angular_velocity = angular_kp*angular_p_error + angular_kd*angular_d_error;
			}
	}
