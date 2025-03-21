#ifndef __UTILL_H__
#define __UTILL_H__

#include "typedef.h"

typedef struct _tag_plc_network_address {
	byte network_number; // Network number, usually 0; configured according to PLC configuration
	byte station_number; // Network station number, usually 0; configured according to PLC configuration
} plc_network_address;

typedef struct _tag_byte_array_info {
	byte* data; // Content
	int length; // Length
} byte_array_info;

typedef struct _tag_bool_array_info {
	bool* data; // 内容
	int length; // Length
} bool_array_info;

void short2bytes(short i, byte* bytes);
short bytes2short(byte* bytes);

void ushort2bytes(ushort i, byte* bytes);
ushort bytes2ushort(byte* bytes);

void int2bytes(int32 i, byte* bytes);
int32 bytes2int32(byte* bytes);

void uint2bytes(uint32 i, byte* bytes);
uint32 bytes2uint32(byte* bytes);

void bigInt2bytes(int64 i, byte* bytes);
int64 bytes2bigInt(byte* bytes);

void ubigInt2bytes(uint64 i, byte* bytes);
uint64 bytes2ubigInt(byte* bytes);

void float2bytes(float i, byte* bytes);
float bytes2float(byte* bytes);

void double2bytes(double i, byte* bytes);
double bytes2double(byte* bytes);

#ifndef _WIN32
char* itoa(unsigned long long value, char str[], int radix);
#endif // !_WIN32

#endif // __UTILL_H__
