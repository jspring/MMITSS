/* sim_spat.c: Reads CA SPaT data from file and re-transmits
**   them to stdout with 200 ms TSCP timing to simulate realtime data stream
*/

#include <sys_os.h>
#include "msgs.h"
#include "sys_rt_linux.h"
#include "ab3418_libudp.h"
#include "ab3418commudp.h"
#include <udp_utils.h>
#include "local.h"
#include <malloc.h>

#define BUFSIZE 3000

int main(int argc, char *argv[]) {

	FILE *fp;
	int fdin;
	int fdout = STDOUT_FILENO;
	char *filename;
	unsigned char *readBuff;
	struct stat buf;
	int start_index = 0;
	int i = 0;

	stat(argv[1], &buf);
	readBuff = calloc((int)buf.st_size, 1);
	fdin = open(argv[1], O_RDONLY);
	read(fdin, readBuff, (int)buf.st_size); 
	for(i=0; i<(int)buf.st_size; i++){
		if( (readBuff[i] == 0x7e) && (readBuff[i+1] == 0x05) &&
			(readBuff[i+3] == 0x13)) {
				start_index = i;
				break;
		}
	}

	while(1) {
		for(i=start_index; i<(int)buf.st_size; i+=sizeof(raw_signal_status_msg_t)) {
			write(fdout, &readBuff[start_index + i], sizeof(raw_signal_status_msg_t));
			fprintf(stderr, "%d %d %d\n", sizeof(raw_signal_status_msg_t), (int)buf.st_size, start_index + i);
			usleep(200000);
		}
	}

}
