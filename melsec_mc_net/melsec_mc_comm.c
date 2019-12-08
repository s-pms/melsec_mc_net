#include "melsec_mc_comm.h"
#include <stdlib.h>
#include <string.h>

melse_mc_data_type mc_create_data_type(byte code, byte type, const char* ascii_code, int from_base)
{
	melse_mc_data_type data_type;
	memset((void*)&data_type, 0, sizeof(data_type));
	data_type.data_code = code;
	data_type.data_type = type;
	strcpy(data_type.ascii_code, ascii_code);
	data_type.from_base = from_base;
	return data_type;
}

int mc_convert_string_to_int(const char* address, int frombase)
{
	int ret = 0;
	ret = (int)strtol(address, NULL, frombase);
	return ret;
}

melsec_mc_address_data mc_analysis_address(const char* address, int length)
{
	melsec_mc_address_data address_data;
	memset((void*)&address_data, 0, sizeof(address_data));
	address_data.length = length;
	switch (address[0])
	{
	case 'M':
	case 'm':
	{
		// M中间继电器
		address_data.data_type = mc_create_data_type(0x90, 0x01, "M*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'X':
	case 'x':
	{
		// X输入继电器
		address_data.data_type = mc_create_data_type(0x9C, 0x01, "X*", 16);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'Y':
	case 'y':
	{
		//  Y输出继电器
		address_data.data_type = mc_create_data_type(0x9D, 0x01, "Y*", 16);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'D':
	case 'd':
	{
		// D数据寄存器
		address_data.data_type = mc_create_data_type(0xA8, 0x01, "D*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'W':
	case 'w':
	{
		// W链接寄存器
		address_data.data_type = mc_create_data_type(0xB4, 0x00, "W*", 16);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'L':
	case 'l':
	{
		// L锁存继电器
		address_data.data_type = mc_create_data_type(0x92, 0x01, "L*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'F':
	case 'f':
	{
		// F报警器
		address_data.data_type = mc_create_data_type(0x93, 0x01, "F*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'V':
	case 'v':
	{
		// V边沿继电器
		address_data.data_type = mc_create_data_type(0x94, 0x01, "V*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'B':
	case 'b':
	{
		// B链接继电器
		address_data.data_type = mc_create_data_type(0xA0, 0x01, "B*", 16);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'R':
	case 'r':
	{
		// R文件寄存器
		address_data.data_type = mc_create_data_type(0xAF, 0x01, "R*", 10);
		const char* real_addr = address + 1;
		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'S':
	case 's':
	{
		const char* real_addr = address + 1;
		if (address[1] == 'N' || address[1] == 'n')
		{
			// 累计定时器的当前值
			address_data.data_type = mc_create_data_type(0xC8, 0x00, "SN", 100);
			real_addr = address + 2;
		}
		else if (address[1] == 'S' || address[1] == 's')
		{
			// 累计定时器的触点
			address_data.data_type = mc_create_data_type(0xC7, 0x01, "SS", 10);
			real_addr = address + 2;
		}
		else if (address[1] == 'C' || address[1] == 'c')
		{
			// 累计定时器的线圈
			address_data.data_type = mc_create_data_type(0xC6, 0x01, "SC", 10);
			real_addr = address + 2;
		}
		else
		{
			// S步进继电器
			address_data.data_type = mc_create_data_type(0x98, 0x01, "S*", 10);
		}

		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'Z':
	case 'z':
	{
		const char* real_addr = address + 1;
		if (address[1] == 'R' || address[1] == 'r')
		{
			// 文件寄存器ZR区
			address_data.data_type = mc_create_data_type(0xB0, 0x00, "ZR", 16);
			real_addr = address + 2;
		}
		else
		{
			// 变址寄存器
			address_data.data_type = mc_create_data_type(0xCC, 0x00, "Z*", 10);
		}

		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'T':
	case 't':
	{
		const char* real_addr = address + 2;
		if (address[1] == 'C' || address[1] == 'c')
		{
			// 定时器的线圈
			address_data.data_type = mc_create_data_type(0xC0, 0x01, "TC", 10);
		}
		else if (address[1] == 'S' || address[1] == 's')
		{
			// 定时器的触点
			address_data.data_type = mc_create_data_type(0xC1, 0x01, "TS", 10);
		}
		else if (address[1] == 'N' || address[1] == 'n')
		{
			// 定时器的当前值
			address_data.data_type = mc_create_data_type(0xC2, 0x01, "TN", 10);
		}

		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	case 'C':
	case 'c':
	{
		const char* real_addr = address + 2;
		if (address[1] == 'N' || address[1] == 'n')
		{
			// 计数器的当前值
			address_data.data_type = mc_create_data_type(0xC5, 0x01, "CN", 10);
		}
		else if (address[1] == 'S' || address[1] == 's')
		{
			//  计数器的触点
			address_data.data_type = mc_create_data_type(0xC4, 0x01, "CS", 10);
		}
		else if (address[1] == 'C' || address[1] == 'c')
		{
			// 计数器的线圈
			address_data.data_type = mc_create_data_type(0xC3, 0x01, "CC", 10);
		}

		address_data.address_start = mc_convert_string_to_int(real_addr, address_data.data_type.from_base);
		break;
	}
	}
	return address_data;
}