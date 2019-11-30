#ifndef	__MCCOMMAND_H__
#define __MCCOMMAND_H__

//////////////////////////////////////////////////////////////////////////
//三菱PLC通讯类，采用Qna兼容3E帧协议实现，需要在PLC侧先的以太网模块先进行配置
//////////////////////////////////////////////////////////////////////////

#include "utill.h"

//协议类型
enum McProtocolType
{
	MC3E = 0x01,
	MC4E = 0x02,

	ASCII_FLAG = 0x80,

	MC3E_BINARY = MC3E,
	MC3E_ASCII = MC3E | ASCII_FLAG,
};

// PLC地址枚举类型
enum PlcDeviceType
{
	ERROR = -1,
	M = 0x90,
	SM = 0x91,
	L = 0x92,
	F = 0x93,
	V = 0x94,
	S = 0x98,
	X = 0x9C,
	Y = 0x9D,
	B = 0xA0,
	SB = 0xA1,
	DX = 0xA2,
	DY = 0xA3,
	D = 0xA8,
	SD = 0xA9,
	R = 0xAF,
	ZR = 0xB0,
	W = 0xB4,
	SW = 0xB5,
	TC = 0xC0,
	TS = 0xC1,
	TN = 0xC2,
	CC = 0xC3,
	CS = 0xC4,
	CN = 0xC5,
	SC = 0xC6,
	SS = 0xC7,
	SN = 0xC8,
	Z = 0xCC,
	TT,
	TM,
	CT,
	CM,
	A,
	Max
};

struct mc_header {
	enum McProtocolType	protocol_type;
	uint32_t		serial_number;		//
	uint8_t			netwrok_number;		// 网络编号
	uint8_t			plc_number;          // PC编号/PLC编号
	uint32_t		io_number;          // 请求目标模块IO编号
	uint8_t			channel_number;     // 请求目标模块站编号
	uint32_t		data_length;		//数据长度
	uint32_t		cpu_timer;          // CPU监视定时器
	int32_t			result_code;        // 返回代码（如果没有返回，则为0xcccc）
};

struct mc_address
{
	int  addr_start;	//地址起始地址（偏移地址）
	unsigned short length; //读取地址长度
	enum PlcDeviceType addr_type;	//地址类型
};

void init_header(struct mc_header *header);

int parse_addr(const char* address, struct mc_address* mc_addr);

byte* mc_build_read_bin_command(struct mc_address address, struct mc_header header, int isbit);
byte* mc_build_read_ascii_command(struct mc_address address, struct mc_header header);

#endif //__MCCOMMAND_H__