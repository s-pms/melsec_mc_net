#ifndef __UTILL_H__
#define __UTILL_H__

#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte;
typedef unsigned short ushort;
typedef signed int	int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

typedef struct _tag_plc_network_address {
	byte	network_number; // ����ţ�ͨ��Ϊ0;����PLC�����ö�����
	byte	station_number; // ����վ�ţ�ͨ��Ϊ0;����PLC�����ö�����
}plc_network_address;

typedef struct _tag_byte_array_info {
	byte* data;	// ����
	int length; // ����
}byte_array_info;

typedef struct _tag_bool_array_info {
	bool* data;	// ����
	int length; // ����
}bool_array_info;

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

#ifndef WIN32
char* itoa (unsigned long long  value,  char str[],  int radix);
#endif // !WIN32


#endif