The Data Manager has 3 principal tasks.

1. Maintain complete database of current data
2. Timestamp all data with acquisition and usage times
3. Serve as watchdog for system health

Startup
	Create database for all requested modules (each struct must have acquisition AND usage TSs).
	Open socket & try to accept connections from all clients
	

while
	Try to accept connections from clients that did not connect at startup
	Receive and ts incoming data
	Push outgoing data
