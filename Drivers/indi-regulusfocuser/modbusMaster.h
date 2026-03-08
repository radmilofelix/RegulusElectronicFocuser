#ifndef MODBUSMASTER___H
#define MODBUSMASTER___H


#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <modbus/modbus.h>


//#define MBMDEBUG

class ModbusMaster
{
	public:
	ModbusMaster(const char * ip, int port);
	ModbusMaster(const char *device, int baud, char parity, int data_bits, int stop_bits);
	~ModbusMaster();
	void MemoryAllocation();
	int Connect();
	void SetSlave(int slave);
	void SetResponseTimeout(time_t seconds, suseconds_t microseconds);
	int WriteRegister(int remoteBufferAddress, int regValue);
	int ReadRegisters(int remoteBufferAddress, int numberOfRegisters,  int localBufferAddress);
	int WriteRegisters(int remoteBufferAddress, int numberOfRegisters,  int localBufferAddress);
	
	
	modbus_t *ctx;
	uint8_t *bit_buffer;
	uint16_t *registry_buffer;	
	int maxRetries;
//	int itemsToProcess;
	
	protected:
};


#endif
