#include "sys_os.h"

int OpenServerListener(char *local_ip, char *remote_ip, unsigned short rcv_port);
int CloseServerListener(int sockfd);
