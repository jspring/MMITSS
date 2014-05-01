
//Yiheng Fneg 04/13/2014


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

char buf[500000];  //The Trajectory message from MRP_EquippedVehicleTrajecotryAware

char temp_log[512];
int continue_while=0;


//define log file name
char predir [64] = "/nojournal/bin/";
char logfilename[256] = "/nojournal/bin/log/MMITSS_rsu_PerformanceObserver_Gavilan.log";
char ConfigInfo[256]	  = "/nojournal/bin/ConfigInfo.txt";

//new map file
char MAP_File_Name[128]="/nojournal/bin/Daysi_Gav_Reduced.nmap";  //file stored in default folder
vector<LaneNodes> MAP_Nodes;

int  outputlog(char *output);
//void get_ip_address();           // READ the virtual ASC controller IP address into INTip from "IPInfo.txt"

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
char *INTport = "501";   //this port is for the virtual controller
int CombinedPhase[8]={0};


string RSUID;
char rsuid_filename[64]   = "/nojournal/bin/rsuid.txt";
char ConfigFile[256] = "/nojournal/bin/ConfigInfo_Daisy_Mout_Traj.txt";  //this is just the temporary file for trajectory based test

LinkedList <ConnectedVehicle> trackedveh;

double GetSeconds();

void UnpackTrajData(byte* ablob);  //This function unpack the trajectory message and save to trackedveh list;

int main ( int argc, char* argv[] )
{

	
	
	
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
	sendaddr.sin_port = htons(22222);  //*** IMPORTANT: the trajectory pushing code should also have this port. ***//
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
		
	while (true)
	{
   
		recv_data_len = recvfrom(sockfd, buf, sizeof(buf), 0,
                        (struct sockaddr *)&sendaddr, (socklen_t *)&addr_length);

		if(recv_data_len<0)
		{
			printf("Receive Request failed\n");

			continue;
		}
		
		sprintf(temp_log,"Received Trajectory Data!\n");
		outputlog(temp_log); cout<<temp_log;
		
		trackedveh.ClearList(); //clear the list for new set of data
		
		//Save the message to the trackedveh list
		UnpackTrajData(buf);
		
		sprintf(temp_log,"The Number of Vehicle Received is: %d\n",trackedveh.ListSize());
		outputlog(temp_log); cout<<temp_log;
		
		/*  This part is for testing and validating the received data
		trackedveh.Reset();
		for(i=0;i<trackedveh.Data().nFrame;i++)
		{
			sprintf(temp_log,"%lf %lf\n",trackedveh.Data().N_Offset[i],trackedveh.Data().E_Offset[i]);
			outputlog(temp_log); cout<<temp_log;
		}
		*/
		
		
		
		//Do the performance observation here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	
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

double GetSeconds()
{
	struct timeval tv_tt;
	gettimeofday(&tv_tt, NULL);
	return (tv_tt.tv_sec+tv_tt.tv_usec/1.0e6);    
}

void UnpackTrajData(byte* ablob)
{
	int No_Veh;
	int i,j;
	int offset;
	offset=0;
	unsigned short   tempUShort; // temp values to hold data in final format
	long    tempLong;
	unsigned char   byteA;  // force to unsigned this time,
	unsigned char   byteB;  // we do not want a bunch of sign extension 
	unsigned char   byteC;  // math mucking up our combine logic
	unsigned char   byteD;
	
	//Header
	byteA = ablob[offset+0];
	byteB = ablob[offset+1];
	int temp = (int)(((byteA << 8) + (byteB))); // in fact unsigned
	offset = offset + 2;
	
	//cout<<temp<<endl;
	
	//id
	temp = (byte)ablob[offset];
	offset = offset + 1; // move past to next item
	//cout<<temp<<endl;
	
	
	//Do vehicle number
	byteA = ablob[offset+0];
	byteB = ablob[offset+1];
	No_Veh = (int)(((byteA << 8) + (byteB))); // in fact unsigned
	offset = offset + 2;
	
	//cout<<No_Veh<<endl;
	
	//Do each vehicle
	for(i=0;i<No_Veh;i++)
	{
		ConnectedVehicle TempVeh;
		//Do Veh ID
		byteA = ablob[offset+0];
		byteB = ablob[offset+1];
		TempVeh.TempID = (int)(((byteA << 8) + (byteB))); // in fact unsigned
		offset = offset + 2;
		//Do nFrame
		byteA = ablob[offset+0];
		byteB = ablob[offset+1];
		TempVeh.nFrame = (int)(((byteA << 8) + (byteB))); // in fact unsigned
		offset = offset + 2;
		//Do the N_offset and E_Offset
		
		//cout<<"nFrame is: "<<TempVeh.nFrame<<endl;
		
		for(j=0;j<TempVeh.nFrame;j++)
		{
			//N_Offset
			byteA = ablob[offset+0];
			byteB = ablob[offset+1];
			byteC = ablob[offset+2];
			byteD = ablob[offset+3];
			tempLong = (unsigned long)((byteA << 24) + (byteB << 16) + (byteC << 8) + (byteD));
			TempVeh.N_Offset[j] = (tempLong /  DEG2ASNunits); // convert and store as float
			offset = offset + 4;
			
			//cout<<"N_Offset["<<j<<"] is:"<<TempVeh.N_Offset[j]<<endl;
			
			//E_Offset
			byteA = ablob[offset+0];
			byteB = ablob[offset+1];
			byteC = ablob[offset+2];
			byteD = ablob[offset+3];
			tempLong = (unsigned long)((byteA << 24) + (byteB << 16) + (byteC << 8) + (byteD));
			TempVeh.E_Offset[j] = (tempLong /  DEG2ASNunits); // convert and store as float
			offset = offset + 4;
			
			//cout<<"E_Offset["<<j<<"] is:"<<TempVeh.E_Offset[j]<<endl;
		}
		//cout<<"Done with one vehicle"<<endl;
		trackedveh.InsertRear(TempVeh);   //add the new vehicle to the tracked list	
	}
	
}


























