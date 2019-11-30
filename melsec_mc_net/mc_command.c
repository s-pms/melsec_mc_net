#include "mc_command.h"
#include <string.h>
#include <stdlib.h>

void init_header(struct mc_header *header)
{
	if (header != NULL)
	{
		header->netwrok_number = 0x00;
		header->serial_number = 0x0001;
		header->plc_number = 0xff;
		header->io_number = 0x03ff;
		header->channel_number = 0x00;
		header->cpu_timer = 0x0010;
		header->data_length = 0x0c;
	}
}

int parse_addr(const char* address, struct mc_address* mc_addr)
{
	int ret = 0;
	if (address == NULL || mc_addr == NULL)
		return ret;
	if (strlen(address) > 1)//地址至少形如：M0
	{
		switch (address[0])
		{
		case 'M':
		case 'm':
		{
			mc_addr->addr_type = M;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'X':
		case 'x':
		{
			mc_addr->addr_type = X;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'Y':
		case 'y':
		{
			mc_addr->addr_type = Y;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'D':
		case 'd':
		{
			mc_addr->addr_type = D;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'W':
		case 'w':
		{
			mc_addr->addr_type = W;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'L':
		case 'l':
		{
			mc_addr->addr_type = L;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'F':
		case 'f':
		{
			mc_addr->addr_type = F;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'V':
		case 'v':
		{
			mc_addr->addr_type = V;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'B':
		case 'b':
		{
			mc_addr->addr_type = B;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'R':
		case 'r':
		{
			mc_addr->addr_type = R;
			mc_addr->addr_start = atoi(address + 1);
			break;
		}
		case 'S':
		case 's':
		{
			if (address[1] == 'N' || address[1] == 'n')
			{
				mc_addr->addr_type = SN;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'S' || address[1] == 's')
			{
				mc_addr->addr_type = SS;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'C' || address[1] == 'c')
			{
				mc_addr->addr_type = SC;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else
			{
				mc_addr->addr_type = S;
				mc_addr->addr_start = atoi(address + 1);
				break;
			}
		}
		case 'Z':
		case 'z':
		{
			if (address[1] == 'R' || address[1] == 'r')
			{
				mc_addr->addr_type = ZR;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else
			{
				mc_addr->addr_type = Z;
				mc_addr->addr_start = atoi(address + 1);
				break;
			}
		}
		case 'T':
		case 't':
		{
			if (address[1] == 'N' || address[1] == 'n')
			{
				mc_addr->addr_type = TN;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'S' || address[1] == 's')
			{
				mc_addr->addr_type = TS;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'C' || address[1] == 'c')
			{
				mc_addr->addr_type = TC;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else
			{
				ret = -1;
				//not support
			}
		}
		case 'C':
		case 'c':
		{
			if (address[1] == 'N' || address[1] == 'n')
			{
				mc_addr->addr_type = CN;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'S' || address[1] == 's')
			{
				mc_addr->addr_type = CS;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else if (address[1] == 'C' || address[1] == 'c')
			{
				mc_addr->addr_type = CC;
				mc_addr->addr_start = atoi(address + 2);
				break;
			}
			else
			{
				ret = -1;
				//Not Supported
			}
		}
		default:
		{
			ret = -1;
			// not supported
		}
		break;
		}
	}

	return ret;
}

byte* mc_build_read_bin_command(struct mc_address address, struct mc_header header, int isbit)
{
	byte* command = (byte*)malloc(10 + 11);//core command + header command
	memset(command, 0, 21);

	command[0] = 0x50;					//副标题
	command[1] = 0x00;
	command[2] = header.netwrok_number;	// 网络号
	command[3] = header.plc_number;		// PLC编号
	command[4] = (byte)header.io_number;// 目标模块IO编号
	command[5] = (byte)(header.io_number >> 8);
	command[6] = header.channel_number;	// 目标模块站号
	command[7] = header.data_length;	// 请求数据长度
	command[8] = (byte)(header.data_length >> 8);
	command[9] = (byte)header.cpu_timer;// CPU监视定时器
	command[10] = (byte)(header.cpu_timer >> 8);

	command[11] = 0x01;					// 批量读取数据命令
	command[12] = 0x04;
	command[13] = isbit ? (byte)0x01 : (byte)0x00;  // 以点为单位还是字为单位成批读取
	command[14] = 0x00;
	command[15] = (byte)address.addr_start; // 起始地址的地位
	command[16] = (byte)(address.addr_start >> 8);
	command[17] = (byte)(address.addr_start >> 16);
	command[18] = (byte)address.addr_type;  // 指明读取的数据
	command[19] = (byte)(address.length);	// 软元件的长度
	command[20] = (byte)(address.length >> 8);

	return command;
}

byte* mc_build_read_ascii_command(struct mc_address address, struct mc_header header)
{
	return NULL;
}