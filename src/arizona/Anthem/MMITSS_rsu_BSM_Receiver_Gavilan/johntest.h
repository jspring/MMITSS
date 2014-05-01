#include "/home/path/db/db_include.h"
//#include "/home/path/local/sys_os.h"

typedef unsigned char bool;

typedef struct {
                bool rf_BrakesApplied ;
                bool lf_BrakesApplied ;
                bool rr_BrakesApplied ;
                bool lr_BrakesApplied ;
                bool wheelBrakesUnavialable ;
                bool spareBit ; //set to 0

                bool traction_1 ;
                bool traction_2 ;
                bool anitLock_1 ;
                bool antiLock_2 ;
                bool stability_1 ;
                bool stability_2 ;
                bool boost_1 ;
                bool boost_2 ;
                bool auxBrakes_1 ;
                bool auxBrakes_2 ;
} IS_PACKED Brakes;

typedef struct {
        double longAcceleration; // -x- Along the Vehicle Longitudinal axis
        double latAcceleration ; //-x- Along the Vehicle Lateral axis
        double verticalAcceleration ; // -x- Along the Vehicle Vertical axis
        double yawRate ;
} IS_PACKED AccelerationSet4Way;

typedef struct {
        double latitude ; // 4 bytes measured in degrees
        double longitude ; // 4 bytes measured in degrees
        double elevation ;  // 2 bytes measured in meters
        double positionAccuracy ; //4 bytes measured in ?? is this J2735 position accurracy??
} IS_PACKED PositionLocal3D;

typedef struct {

        /* Attributes of the SAE J2735 Basic Safety Message
        Referenced to the J2735 Ballot Ready Version 35 */
        // Not Used (is a constant): int DSRCmsgID ;   // 1 byte
        int MsgCount ;    //    1 byte // value is 0-127
        long TemporaryID ; // id TemporaryID, -x- 4 bytes // call it vehicle ID (from VISSIM for simulation)
        int DSecond ;     //secMark DSecond, -x- 2 bytes - Time in milliseconds within a minute

        PositionLocal3D  pos;

        // motion Motion,
        double speed ;    // TransmissionAndSpeed, speed in meters per second
        double heading ;  // Heading, -x- 2 byte compass heading in **degrees**
        /* 9052 Use: The current heading of the sending device, expressed in unsigned units of 0.0125 degrees from North
           9053 (such that 28799 such degrees represent 359.9875 degrees). North shall be defined as the axis defined by
           9054 the WSG-84 coordinate system and its reference ellipsoid. Headings "to the east" are defined as the
           9055 positive direction. A 2 byte value when sent, a value of 28800 shall be used when unavailable. When sent
           9056 by a vehicle, this element indicates the orientation of the front of the vehicle. */
        double angle ;    // SteeringWheelAngle -x- 1 bytes //VISSIM this is lane heading angle
        AccelerationSet4Way accel ;

        // control Control,
        Brakes brakes ; // BrakeSystemStatus

        //-- basic VehicleBasic,
        float length ;   // combined into VehicleSize, 3 bytes
        float width ;
        float weight ; //mass

/*      This section contains data that would be populated a call the intersection
    class with current MAP and SPaT data for the purposes of traffic signal control
    (passing the TemporaryID to it to idenify this instacne of this class)
    */

        int desiredPhase ; //traffic signal phase associated with a lane (MAP + phase association)
        double stopBarDistance ; //distance to the stop bar from current location (MAP)
        double estArrivalTime ; // estimated arrival time at the stop bar given current speed

} IS_PACKED mybsm_t;
