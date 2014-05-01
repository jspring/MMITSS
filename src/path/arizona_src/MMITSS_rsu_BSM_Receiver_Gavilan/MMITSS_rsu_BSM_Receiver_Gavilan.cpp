
//Yiheng Feng 04/13/2014


// Work with Vissim 6 DriverModel through UDP
// Need drivermodel_udp_R.dll from "DriverModel_DLL_UDP_InFusion" running

//2014.4.13: Added function
//Push trajectory data to MRP_PerformanceObserver Component every 30s


//#include <libeloop.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <unistd.h>
                         
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <istream>
#include "math.h"
#include "time.h"
#include <time.h>
#include <vector>
#include <sys/time.h>

#include <iomanip>
#include <stddef.h>
#include <dirent.h>

//#include "Mib.h"
#include "LinkedList.h"
#include "NMAP.h"
#include "geoCoord.h"
#include "BasicVehicle.h"
#include "ListHandle.h"
#include "ConnectedVehicle.h"
//#include "Signal.h"
//#include "Array.h"
//#include "Config.h"   // <vector>
//#include "Print2Log.h"
//#include "GetInfo.h"
//#include "COP.h"

    
using namespace std;

#ifndef BSM_MSG_SIZE
    #define BSM_MSG_SIZE  (45)
#endif

#ifndef DEG2ASNunits
    #define DEG2ASNunits  (10000000.0)  // used for ASN 1/10 MICRO deg to unit converts
#endif

//socket settings
#define PRPORT 15020
//#define OBU_ADDR "192.168.1.25"
#define BROADCAST_ADDR "192.168.101.255"   //DSRC

#define EV  1
#define TRANSIT 2

#define ACTIVE 1
#define NOT_ACTIVE -1
#define LEAVE 0

#define PI 3.14159265

char vehicleid [64];
char buf[256];

char temp_log[512];
int continue_while=0;


//define log file name
char predir [64] = "/nojournal/bin/";
char logfilename[256] = "/nojournal/bin/log/MMITSS_rsu_BSM_receiver.log";
char rndf_file[64]="/nojournal/bin/RNDF.txt";
char active_rndf_file[128]="/nojournal/bin/ActiveRNDF.txt";
char ConfigInfo[256]	  = "/nojournal/bin/ConfigInfo.txt";
char arrivaltablefile[256]= "/nojournal/bin/ArrivalTable.txt";
char trajdata[256]= "/nojournal/bin/trajectorydata.txt";

//new map file
char MAP_File_Name[128]="/nojournal/bin/Daysi_Gav_Reduced.nmap";  //file stored in default folder
vector<LaneNodes> MAP_Nodes;

int  outputlog(char *output);
//void get_ip_address();           // READ the virtual ASC controller IP address into INTip from "IPInfo.txt"
void FindVehInMap(double Speed, double Heading,int nFrame,double N_Offset1,double E_Offset1,double N_Offset2,double E_Offset2, MAP NewMap, double &Dis_curr, double &est_TT, int &request_phase);

geoCoord geoPoint;
double ecef_x,ecef_y,ecef_z;					
//Important!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//Note the rectangular coordinate system: local_x is N(+) S(-) side, and local_y is E(+) W(-) side 
//This is relative distance to the ref point!!!!
//local z can be considered as 0 because no elevation change.
double local_x,local_y,local_z;

//This is the output of the FindVehInMap
double Dis_curr=0;  //current distance to the stop bar
double ETA=0;       //estimated travel time to stop bar
int requested_phase=-1;  //requested phase

//for NTCIP signal control

char IPInfo[64]="/nojournal/bin/ntcipIP.txt";  //stores the virtual asc3 Controller IP address
char INTip[64];
char *INTport = "503";   //this port is for the virtual controller
int CombinedPhase[8]={0};


string RSUID;
char rsuid_filename[64]   = "/nojournal/bin/rsuid.txt";
char ConfigFile[256] = "/nojournal/bin/ConfigInfo_Daisy_Mout_Traj.txt";  //this is just the temporary file for trajectory based test

//For sending the trajectory data
int traj_data_size;  //in byte

LinkedList <ConnectedVehicle> trackedveh;

void PackTrajData(byte* tmp_traj_data,int &size);



//data structure to store the lane nodes and phase mapping

int LaneNode_Phase_Mapping[8][8][20];           //8 approaches, at most 8 lanes each approach, at most 20 lane nodes each lane
												// the value is just the requested phase of this lane node;


//Parameters for trajectory control
int ArrivalTable[121][8];     //maximum planning time horizon * number of phases
int currenttime;

double GetSeconds();

int main ( int argc, char* argv[] )
{

	//double lower_ETA=6; //default value
	//double upper_ETA=8;
	
	int traj_send_freq=10;
	
	//self defined value for sending frequency
	if (argc>=2)	{sscanf(argv[1],"%d",&traj_send_freq);}   

	
	//get_ip_address();           // READ the ASC controller IP address into INTip from "ntcipIP.txt"
	
	
	
	int i,j,k;
	
	//read the new map from the .nmap file and save to NewMap;

	MAP NewMap;
	NewMap.ID=1;
	NewMap.Version=1;
	// Parse the MAP file
	NewMap.ParseIntersection(MAP_File_Name);	

	sprintf(temp_log,"Read the map successfully At (%d).\n",time(NULL));
	outputlog(temp_log); cout<<temp_log;
	
	//Initialize the ref point
	double ref_lat=NewMap.intersection.Ref_Lat;
	double ref_long=NewMap.intersection.Ref_Long;
	double ref_ele=NewMap.intersection.Ref_Ele/10;
	geoPoint.init(ref_long, ref_lat, ref_ele);
	
	sprintf(temp_log,"%lf %lf %lf \n",ref_lat,ref_long,ref_ele);
	outputlog(temp_log); cout<<temp_log;
	
	int temp_phase=0;
	
	for(i=0;i<8;i++)
{
	for(j=0;j<8;j++)
	{
		for(k=0;k<20;k++)
		{
			LaneNode_Phase_Mapping[i][j][k]=0;
		}
	}
}
	
	
	//store all nodes information to MAP_Nodes after parsing the message
	//This is used for calculating vehicle positions in the MAP
	for (i=0;i<NewMap.intersection.Approaches.size();i++)
		for(j=0;j<NewMap.intersection.Approaches[i].Lanes.size();j++)
			for(k=0;k<NewMap.intersection.Approaches[i].Lanes[j].Nodes.size();k++)
			{	
				LaneNodes temp_node;
				temp_node.index.Approach=NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Approach;
				temp_node.index.Lane=NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Lane;
				temp_node.index.Node=NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Node;
				temp_node.Latitude=NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].Latitude;
				temp_node.Longitude=NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].Longitude;
				geoPoint.lla2ecef(temp_node.Longitude,temp_node.Latitude,ref_ele,&ecef_x,&ecef_y,&ecef_z);
				geoPoint.ecef2local(ecef_x,ecef_y,ecef_z,&local_x,&local_y,&local_z);
				temp_node.N_Offset=local_x;
				temp_node.E_Offset=local_y;

				MAP_Nodes.push_back(temp_node);
												
				//sprintf(temp_log,"%d %d %d %lf %lf %lf %lf \n",temp_node.index.Approach,temp_node.index.Lane,temp_node.index.Node,temp_node.N_Offset,temp_node.E_Offset,temp_node.Latitude,temp_node.Longitude);
				//outputlog(temp_log); cout<<temp_log;
			}
	

//Construct the lane node phase mapping matrix
for(int iii=0;iii<MAP_Nodes.size();iii++)
{
	int flag=0;
	for (i=0;i<NewMap.intersection.Approaches.size();i++)
	{
		for(j=0;j<NewMap.intersection.Approaches[i].Lanes.size();j++)
		{
			for(k=0;k<NewMap.intersection.Approaches[i].Lanes[j].Nodes.size();k++)
			{
				if(NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Approach==MAP_Nodes[iii].index.Approach &&
					NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Lane==MAP_Nodes[iii].index.Lane)
				{
					//determine requesting phase
					if (MAP_Nodes[iii].index.Approach==1)  //south bound
					{
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=4;
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=7;
					}
					if (MAP_Nodes[iii].index.Approach==3)
					{
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=6;
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=1;
					}
					if (MAP_Nodes[iii].index.Approach==5)
					{
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=8;
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=3;
					}
					if (MAP_Nodes[iii].index.Approach==7)
					{
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=2;
						if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
							LaneNode_Phase_Mapping[MAP_Nodes[iii].index.Approach][MAP_Nodes[iii].index.Lane][MAP_Nodes[iii].index.Node]=5;
					}
					flag=1;
					break;
				}
			}
			if(flag==1)
				break;
		}
		if(flag==1)
			break;
	}
}

	
				
//for(i=0;i<8;i++)
//{
//	for(j=0;j<8;j++)
//	{
//		for(k=0;k<20;k++)
//		{
//			sprintf(temp_log,"The requested phase for node %d.%d.%d is %d\n",i,j,k,LaneNode_Phase_Mapping[i][j][k]);
//			outputlog(temp_log);
//		}
//	}
//}		
	
	
	//List to store the vehicle trajectory
	ConnectedVehicle TempVeh;


	char BSM_buf[BSM_MSG_SIZE]={0};  //buffer to store the BSM
	BasicVehicle vehIn;

	//------------init: Begin of Network connection------------------------------------
	int sockfd;

	struct sockaddr_in sendaddr;
	struct sockaddr_in recvaddr;
	int numbytes, addr_len;
	int broadcast=1;

	if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) == -1)
	{
		perror("sockfd");
		exit(1);
	}

	if((setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,
		&broadcast,sizeof broadcast)) == -1)
	{
		perror("setsockopt - SO_SOCKET ");
		exit(1);
	}
     
	sendaddr.sin_family = AF_INET;
	sendaddr.sin_port = htons(30000);  //*** IMPORTANT: the vissim should also have this port. ***//
	sendaddr.sin_addr.s_addr = INADDR_ANY;//inet_addr(OBU_ADDR);//INADDR_ANY;

	memset(sendaddr.sin_zero,'\0',sizeof sendaddr.sin_zero);

	if(bind(sockfd, (struct sockaddr*) &sendaddr, sizeof sendaddr) == -1)
	{
		perror("bind");        exit(1);
	}

	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(PRPORT);
	recvaddr.sin_addr.s_addr = inet_addr(BROADCAST_ADDR) ; //INADDR_BROADCAST;
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);

	int addr_length = sizeof ( recvaddr );
	int recv_data_len;
	//-----------------------End of Network Connection------------------//

	cout<<"About to get data from Vissim...............\n";

	int rolling_horizon=1;  //every 1 seconds, write the arrival table to a file
	int begintime=time(NULL);
	currenttime=time(NULL);

	int control_timer;
	
	int counter=0;
	
	double t_1,t_2; //---time stamps used to calculate the time needed
	double traj_send_timer;
	
	traj_send_timer=GetSeconds();
	
	
	while (true)
	{
		//cout<<"BSM from VISSIM No. "<<counter++<<endl;
		//t_1=GetSeconds();
		

		recv_data_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                        (struct sockaddr *)&sendaddr, (socklen_t *)&addr_length);

		if(recv_data_len<0)
		{
			printf("Receive Request failed\n");

			continue;
		}

		for(i=0;i<BSM_MSG_SIZE;i++)  //copy the first 45 bytes to BSM_buf
		{
			BSM_buf[i]=buf[i];
		}

		
		
		
		vehIn.BSM2Vehicle(BSM_buf);  //change from BSM to Vehicle information
		
		//t_2=GetSeconds();
		
		//sprintf(temp_log,"The time for unpacking the BSM is: %lf second \n",t_2-t_1);
		//outputlog(temp_log); cout<<temp_log;
		
		//cout<<setprecision(8)<<vehIn.pos.latitude<<endl;
		
		//process the BSM data with the map information
		int Veh_pos_inlist;
		
		trackedveh.Reset();
		
		int found=0;   //flag to indicate whether a vehicle is found in the list
		int pro_flag=0;  //indicate whether to map the received data in the MAP
		//save vehicle trajactory
		
		//t_1=GetSeconds();
		
		double t1=GetSeconds();
		
		while(!trackedveh.EndOfList())  //match vehicle according to vehicle ID
			{			
				//if ID is a match, store trajectory information
				//If ID will change, then need an algorithm to do the match!!!!
				if(trackedveh.Data().TempID==vehIn.TemporaryID)  //every 0.5s processes a trajectory 
				{
					if (t1-trackedveh.Data().receive_timer>0.5)
					{
						pro_flag=1;   //need to map the point						
						trackedveh.Data().receive_timer=t1;  //reset the timer
						trackedveh.Data().traj[trackedveh.Data().nFrame].latitude=vehIn.pos.latitude;
						trackedveh.Data().traj[trackedveh.Data().nFrame].longitude=vehIn.pos.longitude;
						trackedveh.Data().traj[trackedveh.Data().nFrame].elevation=vehIn.pos.elevation;
						//change GPS points to local points
						geoPoint.lla2ecef(vehIn.pos.longitude,vehIn.pos.latitude,vehIn.pos.elevation,&ecef_x,&ecef_y,&ecef_z);
						geoPoint.ecef2local(ecef_x,ecef_y,ecef_z,&local_x,&local_y,&local_z);
						trackedveh.Data().N_Offset[trackedveh.Data().nFrame]=local_x;
						trackedveh.Data().E_Offset[trackedveh.Data().nFrame]=local_y;
						trackedveh.Data().Speed=vehIn.speed;
						trackedveh.Data().heading=vehIn.heading;
						trackedveh.Data().Dsecond=vehIn.DSecond;
						trackedveh.Data().active_flag=5;  //reset active_flag every time RSE receives BSM from the vehicle
						trackedveh.Data().time[trackedveh.Data().nFrame]=t1;
						
						//sprintf(temp_log,"VehID= %d Latitude= %lf Longitude= %lf Speed= %lf Heading= %lf \n",trackedveh.Data().TempID,
						//trackedveh.Data().traj[trackedveh.Data().nFrame].latitude,trackedveh.Data().traj[trackedveh.Data().nFrame].longitude,trackedveh.Data().Speed,trackedveh.Data().heading);
						//outputlog(temp_log); cout<<temp_log;
						//cout<<"VehID= "<<trackedveh.Data().TempID<<" Speed= "<<trackedveh.Data().Speed<<" Heading= "<<trackedveh.Data().heading<<endl;

						//cout<<"Latitude= "<<trackedveh.Data().traj[trackedveh.Data().nFrame].latitude<<endl;
						//cout<<"Longitude= "<<trackedveh.Data().traj[trackedveh.Data().nFrame].latitude<<endl;
						//cout<<"Elevation= "<<trackedveh.Data().traj[trackedveh.Data().nFrame].elevation<<endl;
						
						
				//		fstream traj;
				//		traj.open(trajdata,ios::out|ios::app);	
				//		int temp_frame=trackedveh.Data().nFrame;
						//for (i=1;i<=temp_frame;i++)
						//{
						
				//			traj<<trackedveh.Data().TempID<<" ";
				//			traj<<fixed<<setprecision(2)<<trackedveh.Data().time[temp_frame];
				//			traj<<" "<<trackedveh.Data().N_Offset[temp_frame]<<" "<<trackedveh.Data().E_Offset[temp_frame]<<" "<<trackedveh.Data().req_phase<<endl;
						//}
				//		traj.close();
						
						
						
						trackedveh.Data().nFrame++; 
						Veh_pos_inlist=trackedveh.CurrentPosition();  //store this vehicle's position in the tracked vehicle list, start from 0
					}
					found=1;
					break;
				}
			trackedveh.Next();
			}
		if(found==0)  //this is a new vehicle
			{
				TempVeh.TempID=vehIn.TemporaryID;
				TempVeh.traj[0].latitude=vehIn.pos.latitude;
				TempVeh.traj[0].longitude=vehIn.pos.longitude;
				TempVeh.traj[0].elevation=vehIn.pos.elevation;
				//change GPS points to local points
				geoPoint.lla2ecef(vehIn.pos.longitude,vehIn.pos.latitude,vehIn.pos.elevation,&ecef_x,&ecef_y,&ecef_z);
				geoPoint.ecef2local(ecef_x,ecef_y,ecef_z,&local_x,&local_y,&local_z);
				TempVeh.N_Offset[0]=local_x;
				TempVeh.E_Offset[0]=local_y;
				TempVeh.Speed=vehIn.speed;
				TempVeh.nFrame=1;
				TempVeh.Phase_Request_Counter=0;
				TempVeh.active_flag=5;  //initilize the active_flag
				TempVeh.receive_timer=GetSeconds();
				TempVeh.time[0]=TempVeh.receive_timer;
				sprintf(temp_log,"Add Vehicle No. %d \n",TempVeh.TempID);
				outputlog(temp_log); cout<<temp_log;
				
				trackedveh.InsertRear(TempVeh);   //add the new vehicle to the tracked list
				Veh_pos_inlist=trackedveh.ListSize()-1;  //start from 0
			}
			
			
	//Find the vehicle in the map: output: distance, eta and requested phase
	
	
	//t_2=GetSeconds();
		
	//sprintf(temp_log,"The time for Adding vehicle to the list is: %lf second \n",t_2-t_1);
	//outputlog(temp_log); cout<<temp_log;
	

	if(pro_flag==1)
	{
		//t_1=GetSeconds();
		trackedveh.Reset(Veh_pos_inlist);
		double tmp_speed=trackedveh.Data().Speed;
		double tmp_heading=trackedveh.Data().heading;
		int tmp_nFrame=trackedveh.Data().nFrame;
		double tmp_N_Offset1=trackedveh.Data().N_Offset[trackedveh.Data().nFrame-1];
		double tmp_E_Offset1=trackedveh.Data().E_Offset[trackedveh.Data().nFrame-1];
		double tmp_N_Offset2=trackedveh.Data().N_Offset[trackedveh.Data().nFrame-2];
		double tmp_E_Offset2=trackedveh.Data().E_Offset[trackedveh.Data().nFrame-2];
		
		
		FindVehInMap(tmp_speed,tmp_heading,tmp_nFrame,tmp_N_Offset1,tmp_E_Offset1,tmp_N_Offset2,tmp_E_Offset2,NewMap,Dis_curr,ETA,requested_phase);		
		trackedveh.Data().req_phase=requested_phase;
		trackedveh.Data().ETA=ETA;
		pro_flag=0;
		
		//t_2=GetSeconds();	
		//sprintf(temp_log,"The time for Processing BSM and Mapping for vehicle is: %lf second \n",t_2-t_1);
		//outputlog(temp_log); cout<<temp_log;
		
		//t_2=GetSeconds();	
		//sprintf(temp_log,"The time for locating the vehicle to the MAP is: %lf second \n",t_2-t_1);
		//outputlog(temp_log); cout<<temp_log;
	
	}
	
	
	//go through the tracked list to create the arrival table every rolling-horizon time
	currenttime=time(NULL);
	int difference=currenttime-begintime;
	
	//sprintf(temp_log,"The Difference is %d\n",difference);
	//outputlog(temp_log); cout<<temp_log;
	
	if (difference>=rolling_horizon)
	{	
	
		//t_1=GetSeconds();
	
		//go throught the vehicle list and delete the already left vehicle
		trackedveh.Reset();
		while(!trackedveh.EndOfList())  //match vehicle according to vehicle ID
		{
			trackedveh.Data().active_flag--;
			if (trackedveh.Data().active_flag<-1)
			{
				//first write the vehicle trajectory to a file
				
				
				
				//then delete the vehicle
				sprintf(temp_log,"Delete Vehicle No. %d \n",trackedveh.Data().TempID);
				outputlog(temp_log); cout<<temp_log;
				trackedveh.DeleteAt();
			}
			trackedveh.Next();
		}
	
		//reset begintime;
		begintime=currenttime;
		//clear current arrival table
		for (i=0;i<121;i++)
			for(j=0;j<8;j++)
			{
				ArrivalTable[i][j]=0;
			}
		
		//construct the arrival table
		trackedveh.Reset();
			while(!trackedveh.EndOfList())
			{
				if(trackedveh.Data().req_phase>0)  //only vehicle request phase, then record the arrival time
				{
					int att=(int) floor(trackedveh.Data().ETA+0.5);
					ArrivalTable[att][trackedveh.Data().req_phase-1]++;
				}
			trackedveh.Next();
			}
			
		//write arrival table to the file
		fstream arr;
		arr.open(arrivaltablefile,ios::out)	;	
		
		for (i=0;i<121;i++)
		{
			for(j=0;j<7;j++)
			{
				arr<<ArrivalTable[i][j]<<" ";
			}
				arr<<ArrivalTable[i][7];
			if(i<120)
			arr<<endl;
		}
		arr.close();
	}
	
	//Push Trajectory data every 30s
	double tmp_time=GetSeconds();
	if(tmp_time-traj_send_timer>=traj_send_freq) //push trajectory to MRP_PerformanceObserver
	{
		traj_send_timer=tmp_time;  //reset traj_send_timer
		//Set-up receive address as local host
		recvaddr.sin_family = AF_INET;
		recvaddr.sin_port = htons(22222);
		recvaddr.sin_addr.s_addr = inet_addr("127.0.0.1") ; //Send to local host;
		memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);
		
		//Pack the trajectory data to a octet stream
		byte tmp_traj_data[500000];  // 500k is the maximum can be sent
		//Pack the data from trackedveh to a octet string
		t_1=GetSeconds();
		PackTrajData(tmp_traj_data,traj_data_size);
		t_2=GetSeconds();
		
		cout<<"Pack time is: "<<t_2-t_1<<endl;
		
		char* traj_data;
		traj_data= new char[traj_data_size];
		
		for(i=0;i<traj_data_size;i++)
		traj_data[i]=tmp_traj_data[i];
		//Send trajectory data
		
		t_1=GetSeconds();
		numbytes = sendto(sockfd,traj_data,traj_data_size+1 , 0,(struct sockaddr *)&recvaddr, addr_length);
		
		t_2=GetSeconds();
		
		sprintf(temp_log,"Send trajectory data! The size is %d and sending time is %lf. \n",traj_data_size,t_2-t_1);
		outputlog(temp_log); cout<<temp_log;
		
		delete[] traj_data;
	}
	   
	
	}// End of while(true)
	//*/
	return 0;

} // ---End of main()

//**********************************************************************************//

int outputlog(char *output)
{
	FILE * stream = fopen( logfilename, "r" );

	if (stream==NULL)
	{
		perror ("Error opening file");
	}

	fseek( stream, 0L, SEEK_END );
	long endPos = ftell( stream );
	fclose( stream );

	fstream fs;
	if (endPos <10000000)
		fs.open(logfilename, ios::out | ios::app);
	else
		fs.open(logfilename, ios::out | ios::trunc);

	//fstream fs;
	//fs.open("/nojournal/bin/OBU_logfile.txt", ios::out | ios::app);
	if (!fs || !fs.good())
	{
		cout << "could not open file!\n";
		return -1;
	}
	fs << output;

	if (fs.fail())
	{
		cout << "failed to append to file!\n";
		return -1;
	}
	fs.close();

	return 1;
}



void FindVehInMap(double Speed, double Heading,int nFrame,double N_Offset1,double E_Offset1,double N_Offset2,double E_Offset2, MAP NewMap, double &Dis_curr, double &est_TT, int &request_phase)
{
            int i,j,k;
			//Find Vehicle position in the map
            //find the nearest Lane nodes from the vehicle location
			int t_App,t_Lane,t_Node;
			int t_pos; //node position in the vector
			double lane_heading;
			//temp vehicle point

			//calculate the vehicle in the MAP
			//find lane, requesting phase and distance 
				int tempdis=100000;
				//find the nearest lane node
				for(j=0;j<MAP_Nodes.size();j++)
				{
					double dis=sqrt((local_x-MAP_Nodes[j].N_Offset)*(local_x-MAP_Nodes[j].N_Offset)+(local_y-MAP_Nodes[j].E_Offset)*(local_y-MAP_Nodes[j].E_Offset));
					if (dis<tempdis)
					{
						tempdis=dis;
						t_App=MAP_Nodes[j].index.Approach;
						t_Lane=MAP_Nodes[j].index.Lane;
						t_Node=MAP_Nodes[j].index.Node;
						t_pos=j;
					}
				}
				
				//trackedveh.Reset(Veh_pos_inlist);
				//sprintf(temp_log,"The nearest node is: %d %d %d \n",t_App,t_Lane,t_Node);
				//outputlog(temp_log); cout<<temp_log;
				if(nFrame>=2) //start from second frame
			{
				// determine it is approaching the intersection or leaving the intersection or in queue
				// The threshold for determing in queue: 89.4 cm = 2mph
				//calculate the distance from the reference point here is 0,0;
				int veh_state; 		// 1: appraoching; 2: leaving; 3: queue
				double N_Pos;  //current vehicle position
				double E_Pos;
				double E_Pos2; //previous vehicle position
				double N_Pos2;

				int match=0;  //whether the vehicle's heading match the lanes heading  

				//find the first node (nearest of intersection) of the lane
				double inter_pos_N=MAP_Nodes[t_pos-t_Node+1].N_Offset;
				double inter_pos_E=MAP_Nodes[t_pos-t_Node+1].E_Offset;


				N_Pos=N_Offset1;//current position
				E_Pos=E_Offset1;
				N_Pos2=N_Offset2;//previous frame position
				E_Pos2=E_Offset2;

				//double veh_heading=atan2(N_Pos-N_Pos2,E_Pos-E_Pos2)*180/PI;
				double veh_heading=Heading;
				if (veh_heading<0)
					veh_heading+=360;
					
				//calculate lane heading
				if (t_Node>1)  //not the nearest node
				{
					double app_node_N=MAP_Nodes[t_pos-1].N_Offset; //the node to the intersection has smaller number
					double app_node_E=MAP_Nodes[t_pos-1].E_Offset;
					lane_heading=atan2(MAP_Nodes[t_pos-1].N_Offset-MAP_Nodes[t_pos].N_Offset,MAP_Nodes[t_pos-1].E_Offset-MAP_Nodes[t_pos].E_Offset)*180.0/PI;
				}
				else  //t_Node=1  //already the nearest node
				{
					double app_node_N=MAP_Nodes[t_pos+1].N_Offset; //the adjacent node has larger number
					double app_node_E=MAP_Nodes[t_pos+1].E_Offset;
					lane_heading=atan2(MAP_Nodes[t_pos].N_Offset-MAP_Nodes[t_pos+1].N_Offset,MAP_Nodes[t_pos].E_Offset-MAP_Nodes[t_pos+1].E_Offset)*180.0/PI;
				}
				if(lane_heading<0)
				lane_heading+=360;
				
				if (abs(veh_heading-lane_heading)<20)   //threshold for the difference of the heading
				match=1;

				double Dis_pre= sqrt((N_Pos2-inter_pos_N)*(N_Pos2-inter_pos_N)+(E_Pos2-inter_pos_E)*(E_Pos2-inter_pos_E));
				Dis_curr=sqrt((N_Pos-inter_pos_N)*(N_Pos-inter_pos_N)+(E_Pos-inter_pos_E)*(E_Pos-inter_pos_E));
				if (fabs(Dis_pre-Dis_curr)<0.894/10 && match==1)  //unit m/0.1s =2mph //Veh in queue is also approaching the intersection, leaving no queue
				{
					veh_state=3;  //in queue
				}
				if (fabs(Dis_pre-Dis_curr)>=0.894/10 && match==1)
				{
					if (Dis_curr<Dis_pre)
						veh_state=1;  //approaching
					else
					{
						veh_state=2;  //leaving
						request_phase=-1;  //if leaving, no requested phase
					}
				}
				if (match==0)
				{
					veh_state=2;
					request_phase=-1;  //if leaving, no requested phase
				}
				//sprintf(temp_log,"Veh State is %d \n",veh_state);
				//outputlog(temp_log); cout<<temp_log;

				//cout<<" Veh State is "<<veh_state<<endl;

				if (veh_state==1 || veh_state==3) //only vehicle approaching intersection need to do something
				{
						request_phase=LaneNode_Phase_Mapping[t_App][t_Lane][t_Node];
						/*
						int flag=0;
						//find the corresponding lane in the MAP
						for (i=0;i<NewMap.intersection.Approaches.size();i++)
						{
							for(j=0;j<NewMap.intersection.Approaches[i].Lanes.size();j++)
							{
								for(k=0;k<NewMap.intersection.Approaches[i].Lanes[j].Nodes.size();k++)
								{
									if(NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Approach==t_App &&
										NewMap.intersection.Approaches[i].Lanes[j].Nodes[k].index.Lane==t_Lane)
									{
										//determine requesting phase
										if (t_App==1)  //south bound
										{
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
												request_phase=4;
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
												request_phase=7;
										}
										else if (t_App==3)
										{
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
												request_phase=6;
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
												request_phase=1;
										}
										else if (t_App==5)
										{
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
												request_phase=8;
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
												request_phase=3;
										}
										else if (t_App==7)
										{
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[1]==1)  //through
												request_phase=2;
											if (NewMap.intersection.Approaches[i].Lanes[j].Attributes[2]==1)  //left turn
												request_phase=5;
										}
										else //leaving the intersection
										cout<<"Vehicle leaving the intersection"<<endl;
										flag=1;
										break;
									}
								}
								if(flag==1)
									break;
							}
							if(flag==1)
								break;
						}
						*/
					//sprintf(temp_log,"Request Phase is %d \n",request_phase);
					//outputlog(temp_log); cout<<temp_log;
					//cout<<"Request Phase is "<<request_phase;
					//cout<<" Current distance to stopbar is "<<Dis_curr;
					//calculate estimated travel time to stop bar
					if(Speed<1)
						est_TT=0;    //if the vehicle is in queue, assume ETA is 0
					else
						est_TT=Dis_curr/Speed;
					if(est_TT>9999)
						est_TT=9999;
					//sprintf(temp_log,"Current distance to stopbar is %lf TT to stop bar is %lf \n",Dis_curr,est_TT);
					//outputlog(temp_log); cout<<temp_log;
					//cout<<" TT to stop bar is "<<est_TT<<endl;

				}

			}
}


double GetSeconds()
{
	struct timeval tv_tt;
	gettimeofday(&tv_tt, NULL);
	return (tv_tt.tv_sec+tv_tt.tv_usec/1.0e6);    
}

void PackTrajData(byte* tmp_traj_data,int &size)
{
	int i,j;
	int offset=0;
	byte*   pByte;      // pointer used (by cast)to get at each byte 
                            // of the shorts, longs, and blobs
    byte    tempByte;   // values to hold data once converted to final format
	unsigned short   tempUShort;
    long    tempLong;
    double  temp;
	//header 2 bytes
	tmp_traj_data[offset]=0xFF;
	offset+=1;
	tmp_traj_data[offset]=0xFF;
	offset+=1;
	//MSG ID: 0x01 for trajectory data
	tmp_traj_data[offset]=0x01;
	offset+=1;
	//No. of Vehicles in the list
	tempUShort = (unsigned short)trackedveh.ListSize();
	
	cout<<"tmpUShort is: "<<tempUShort<<endl;
	
	pByte = (byte* ) &tempUShort;
    tmp_traj_data[offset+0] = (byte) *(pByte + 1); 
    tmp_traj_data[offset+1] = (byte) *(pByte + 0); 
	offset = offset + 2;
	//for each vehicle
	trackedveh.Reset();
	while(!trackedveh.EndOfList())
	{
		//vehicle temporary id
		tempUShort = (unsigned short)trackedveh.Data().TempID;
		pByte = (byte* ) &tempUShort;
		tmp_traj_data[offset+0] = (byte) *(pByte + 1); 
		tmp_traj_data[offset+1] = (byte) *(pByte + 0); 
		offset = offset + 2;
		//the number of trajectory points: nFrame
		tempUShort = (unsigned short)trackedveh.Data().nFrame;
		pByte = (byte* ) &tempUShort;
		tmp_traj_data[offset+0] = (byte) *(pByte + 1); 
		tmp_traj_data[offset+1] = (byte) *(pByte + 0); 
		offset = offset + 2;
		//do each trajectory point
		//this following order: latitude,longitude,N_Offset,E_Offset
		for(j=0;j<trackedveh.Data().nFrame;j++)
		{
			/*
			//latitude
			tempLong = (long)(trackedveh.Data().traj[j].latitude * DEG2ASNunits); 
			pByte = (byte* ) &tempLong;
			tmp_traj_data[offset+0] = (byte) *(pByte + 3); 
			tmp_traj_data[offset+1] = (byte) *(pByte + 2); 
			tmp_traj_data[offset+2] = (byte) *(pByte + 1); 
			tmp_traj_data[offset+3] = (byte) *(pByte + 0); 
			offset = offset + 4;
			//longitude
			tempLong = (long)(trackedveh.Data().traj[j].longitude * DEG2ASNunits); // to 1/10th micro degees units
			pByte = (byte* ) &tempLong;
			tmp_traj_data[offset+0] = (byte) *(pByte + 3); 
			tmp_traj_data[offset+1] = (byte) *(pByte + 2); 
			tmp_traj_data[offset+2] = (byte) *(pByte + 1); 
			tmp_traj_data[offset+3] = (byte) *(pByte + 0); 
			offset = offset + 4;
			*/
			//N_Offset
			tempLong = (long)(trackedveh.Data().N_Offset[j]* DEG2ASNunits); // to 1/10th micro degees units
			pByte = (byte* ) &tempLong;
			tmp_traj_data[offset+0] = (byte) *(pByte + 3); 
			tmp_traj_data[offset+1] = (byte) *(pByte + 2); 
			tmp_traj_data[offset+2] = (byte) *(pByte + 1); 
			tmp_traj_data[offset+3] = (byte) *(pByte + 0); 
			offset = offset + 4;
			//E_Offset
			tempLong = (long)(trackedveh.Data().E_Offset[j] * DEG2ASNunits); // to 1/10th micro degees units
			pByte = (byte* ) &tempLong;
			tmp_traj_data[offset+0] = (byte) *(pByte + 3); 
			tmp_traj_data[offset+1] = (byte) *(pByte + 2); 
			tmp_traj_data[offset+2] = (byte) *(pByte + 1); 
			tmp_traj_data[offset+3] = (byte) *(pByte + 0); 
			offset = offset + 4;
		}
	trackedveh.Next();	
	}
	size=offset;
}



















