#include "modbusMaster.h"

ModbusMaster::ModbusMaster(const char * ip, int port)
{
	ctx = modbus_new_tcp(ip, port);
	maxRetries=100;
	MemoryAllocation();
//	itemsToProcess=items;
}

ModbusMaster::ModbusMaster(const char *device, int baud, char parity, int data_bits, int stop_bits)
{
	ctx = modbus_new_rtu(device, baud, parity, data_bits, stop_bits);
	maxRetries=10;
	MemoryAllocation();
//	itemsToProcess=items;
}

ModbusMaster::~ModbusMaster()
{
    free(bit_buffer);
    free(registry_buffer);
    modbus_close(ctx);
    modbus_free(ctx);
}

void ModbusMaster::MemoryAllocation()
{
    bit_buffer = (uint8_t *) malloc(MODBUS_MAX_READ_BITS * sizeof(uint8_t));
    memset(bit_buffer, 0, MODBUS_MAX_READ_BITS * sizeof(uint8_t));

    registry_buffer = (uint16_t *) malloc(MODBUS_MAX_READ_REGISTERS * sizeof(uint16_t));
    memset(registry_buffer, 0, MODBUS_MAX_READ_REGISTERS * sizeof(uint16_t));
}

int ModbusMaster::Connect()
{
	if (modbus_connect(ctx) == -1)
	{
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
	return 1;
}

void ModbusMaster::SetResponseTimeout(time_t seconds, suseconds_t microseconds)
{
	modbus_set_response_timeout(ctx, seconds, microseconds);
}

void ModbusMaster::SetSlave(int slave)
{
	modbus_set_slave(ctx, slave);
}


int ModbusMaster::WriteRegister(int remoteBufferAddress, int regValue)
{
	int result=1;
	int i;
	for (i=0; i<maxRetries; i++)
	{
		result = modbus_write_register(ctx, remoteBufferAddress, regValue);
		if (result == -1)
		{
			fprintf(stderr, "%s\n", modbus_strerror(errno));
		}
		else
		{
			#ifdef MBMDEBUG
			printf("i = %d;  rc = %d\n",i,result);
			#endif
			break;
		}
	}
	result=i;
	return result;
}


int ModbusMaster::ReadRegisters(int remoteBufferAddress, int numberOfRegisters,  int localBufferAddress)
{
	if( (remoteBufferAddress+numberOfRegisters) > MODBUS_MAX_READ_REGISTERS)
	{
        fprintf(stderr, "Wrong read registers parameters. Max r_registers: %d\n", MODBUS_MAX_READ_REGISTERS);
		return -1;
	}
	int result=1;
	int i;
	for (i=0; i<maxRetries; i++)
	{
		result = modbus_read_registers(ctx, remoteBufferAddress, numberOfRegisters, registry_buffer+localBufferAddress);
		if (result == -1)
		{
			fprintf(stderr, "%s\n", modbus_strerror(errno));
		}
		else
		{
			#ifdef MBMDEBUG
			printf("i = %d;  rc = %d\n",i,result);
			#endif
			break;
		}
	}
	result=i;
	return result;
}

int ModbusMaster::WriteRegisters(int remoteBufferAddress, int numberOfRegisters,  int localBufferAddress)
{
	if( (remoteBufferAddress+numberOfRegisters) > MODBUS_MAX_WRITE_REGISTERS)
	{
        fprintf(stderr, "Wrong write registers parameters. Max w_registers: %d\n", MODBUS_MAX_WRITE_REGISTERS);
		return -1;
	}
	int result=1;
	int i;
	for (i=0; i<maxRetries; i++)
	{
		result = modbus_write_registers(ctx, remoteBufferAddress, numberOfRegisters, registry_buffer+localBufferAddress);
		if (result == -1)
		{
			fprintf(stderr, "%s\n", modbus_strerror(errno));
		}
		else
		{
			#ifdef MBMDEBUG
			printf("i = %d;  rc = %d\n",i,result);
			#endif
			break;
		}
	}
	result=i;
	return result;
}

