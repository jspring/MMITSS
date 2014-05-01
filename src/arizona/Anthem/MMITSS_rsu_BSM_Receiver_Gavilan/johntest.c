#include "johntest.h"

int main( int argc, char *argv[]) {


	mybsm_t bsm;

	memset(&bsm, 0, sizeof(mybsm_t));


	while(1) {

		bsm.MsgCount++;
		bsm.TemporaryID = 1;	
		bsm.DSecond = 0;
		bsm.pos.latitude = 0;
		bsm.pos.longitude = 0;
		bsm.pos.elevation = 0;
		bsm.pos.positionAccuracy = 0;
		bsm.speed = 10.0; //m/s
		bsm.heading = 0;
		bsm.angle = 0;    // SteeringWheelAngle -x- 1 bytes //VISSIM this is lane heading angle
		bsm.accel.longAcceleration= 0;
		bsm.accel.latAcceleration= 0;
		bsm.accel.verticalAcceleration= 0;
		bsm.accel.yawRate = 0;
		bsm.brakes.rf_BrakesApplied = 0;
		bsm.brakes.lf_BrakesApplied = 0;
		bsm.brakes.rr_BrakesApplied = 0;
		bsm.brakes.lr_BrakesApplied = 0;
		if (write(STDOUT_FILENO, &bsm, sizeof(mybsm_t)) != sizeof(mybsm_t))
			perror("write");

		sleep(1);
	}
}
