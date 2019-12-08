#include "melsec_helper.h"
#include "melsec_mc_bin.h"
#include "melsec_mc_bin_private.h"

#include "socket.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#pragma warning(disable:4996)
#else
#include <sys/types.h>
#include <sys/socket.h>
#endif

#define RELEASE_DATA(addr) { if(addr != NULL) { free(addr);} }

#define BUFFER_SIZE 512

int mc_connect(char* ip_addr, int port, byte network_addr, byte station_addr)
{
	int fd = -1;
	g_network_address.network_number = network_addr;
	g_network_address.station_number = station_addr;

	fd = mc_open_tcp_client_socket(ip_addr, port);
	return fd;
}

bool mc_disconnect(int fd)
{
	mc_close_tcp_socket(fd);
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool read_bool_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	bool ret = false;
	melsec_mc_address_data address_data = mc_analysis_address(address, length);
	byte_array_info core_cmd = build_read_core_command(address_data, true);
	if (core_cmd.data != NULL)
	{
		byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_read_response(response, out_bytes);
				if (ret)
					extract_actual_bool_data(out_bytes);
			}
		}
		free(cmd.data);
	}
	return ret;
}

bool read_word_value(int fd, const char* address, int length, byte_array_info* out_bytes)
{
	melsec_mc_address_data address_data = mc_analysis_address(address, length);
	ushort already_finished = 0;
	bool isok = false;
	isok = read_address_data(fd, address_data, out_bytes);
	if (!isok)
		return isok;
	return true;
}

bool read_address_data(int fd, melsec_mc_address_data address_data, byte_array_info* out_bytes)
{
	bool ret = false;
	byte_array_info core_cmd = build_read_core_command(address_data, false);
	if (core_cmd.data != NULL)
	{
		byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_read_response(response, out_bytes);
			}
		}
		free(cmd.data);
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////
bool write_bool_value(int fd, const char* address, int length, bool_array_info in_bytes)
{
	melsec_mc_address_data address_data = mc_analysis_address(address, length);
	bool isok = false;
	byte_array_info core_cmd = build_write_bit_core_command(address_data, in_bytes);
	if (core_cmd.data != NULL)
	{
		byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				isok = mc_parse_write_response(response, NULL);
			}
		}
		free(cmd.data);
	}
	return isok;
}

bool write_word_value(int fd, const char* address, int length, byte_array_info in_bytes)
{
	melsec_mc_address_data address_data = mc_analysis_address(address, length);
	bool isok = false;
	isok = write_address_data(fd, address_data, in_bytes);
	if (!isok)
		return isok;
	return true;
}

bool write_address_data(int fd, melsec_mc_address_data address_data, byte_array_info in_bytes)
{
	bool ret = false;
	byte_array_info core_cmd = build_write_word_core_command(address_data, in_bytes);
	if (core_cmd.data != NULL)
	{
		byte_array_info cmd = pack_mc_command(&core_cmd, g_network_address.network_number, g_network_address.station_number);
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_write_response(response, NULL);
			}
		}
		free(cmd.data);
	}
	return ret;
}

bool mc_remote_run(int fd)
{
	bool ret = false;
	byte core_cmd_temp[] = { 0x01, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp;
	temp.data = core_cmd;
	temp.length = core_cmd_len;
	byte_array_info  cmd = pack_mc_command(&temp, g_network_address.network_number, g_network_address.station_number);
	if (cmd.data != NULL)
	{
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_read_response(response, NULL);
			}
		}
		free(cmd.data);
	}

	return ret;
}

bool mc_remote_stop(int fd)
{
	bool ret = false;
	byte core_cmd_temp[] = { 0x02, 0x10, 0x00, 0x00, 0x01, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp;
	temp.data = core_cmd;
	temp.length = core_cmd_len;
	byte_array_info cmd = pack_mc_command(&temp, g_network_address.network_number, g_network_address.station_number);
	if (cmd.data != NULL)
	{
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_read_response(response, NULL);
			}
		}
		free(cmd.data);
	}

	return ret;
}

bool mc_remote_reset(int fd)
{
	bool ret = false;
	byte core_cmd_temp[] = { 0x06, 0x10, 0x00, 0x00, 0x01, 0x00 };

	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp;
	temp.data = core_cmd;
	temp.length = core_cmd_len;
	byte_array_info cmd = pack_mc_command(&temp, g_network_address.network_number, g_network_address.station_number);
	if (cmd.data != NULL)
	{
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				ret = mc_parse_read_response(response, NULL);
			}
		}
		free(cmd.data);
	}

	return ret;
}

char* mc_read_plc_type(int fd)
{
	bool is_ok = false;
	byte_array_info out_bytes;
	memset(&out_bytes, 0, sizeof(out_bytes));

	byte core_cmd_temp[] = { 0x01, 0x01, 0x00, 0x00 };
	int core_cmd_len = sizeof(core_cmd_temp) / sizeof(core_cmd_temp[0]);
	byte* core_cmd = (byte*)malloc(core_cmd_len);
	memcpy(core_cmd, core_cmd_temp, core_cmd_len);

	byte_array_info temp;
	temp.data = core_cmd;
	temp.length = core_cmd_len;
	byte_array_info cmd = pack_mc_command(&temp, g_network_address.network_number, g_network_address.station_number);
	if (cmd.data != NULL)
	{
		int need_send = cmd.length;
		int real_sends = mc_write_msg(fd, cmd.data, need_send);
		if (real_sends == need_send)
		{
			byte temp[BUFFER_SIZE];
			memset(temp, 0, BUFFER_SIZE);
			byte_array_info response;
			response.data = temp;
			response.length = BUFFER_SIZE;

			int recv_size = mc_read_response(fd, &response);
			int min = 11;
			if (recv_size >= min) //header size
			{
				is_ok = mc_parse_read_response(response, &out_bytes);
			}
		}
		free(cmd.data);
	}

	if (is_ok && out_bytes.length > 0)
	{
		return (char*)out_bytes.data;
	}
	return NULL;
}

byte_array_info pack_mc_command(byte_array_info* mc_core, byte network_number, byte station_number)
{
	int core_len = mc_core->length;
	int cmd_len = core_len + 11;
	byte* command = (byte*)malloc(cmd_len);//core command + header command
	memset(command, 0, core_len + 11);

	command[0] = 0x50;					//副标题
	command[1] = 0x00;
	command[2] = network_number;		// 网络号
	command[3] = 0xFF;					// PLC编号
	command[4] = 0xFF;					// 目标模块IO编号
	command[5] = 0x03;
	command[6] = station_number;		// 目标模块站号
	command[7] = (byte)(cmd_len - 9);	// 请求数据长度
	command[8] = (byte)((cmd_len - 9) >> 8);
	command[9] = 0x0A;					// CPU监视定时器
	command[10] = 0x00;
	memcpy(command + 11, mc_core->data, mc_core->length);

	byte_array_info ret;
	ret.data = command;
	ret.length = cmd_len;
	free(mc_core->data);

	return ret;
}

void extract_actual_bool_data(byte_array_info* response)
{
	// 位读取
	int resp_len = response->length * 2;
	byte* content = (byte*)malloc(resp_len);
	memset(content, 0, resp_len);

	for (int i = 0; i < response->length; i++)
	{
		if ((response->data[i] & 0x10) == 0x10)
			content[i * 2 + 0] = 0x01;

		if ((response->data[i] & 0x01) == 0x01)
			content[i * 2 + 1] = 0x01;
	}
	free(response->data);
	response->data = content;
	response->length = resp_len;
}

int mc_read_response(int fd, byte_array_info* response)
{
	int   nread = 0;
	char* ptr = (char*)response->data;

	if (fd < 0 || response->length <= 0) return -1;

	nread = (int)recv(fd, ptr, response->length, 0);

	if (nread < 0) {
		return -1;
	}
	response->length = nread;

	return nread;
}

//////////////////////////////////////////////////////////////////////////

bool mc_read_bool(int fd, const char* address, bool* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_bool_value(fd, address, 1, &read_data);
	if (ret && read_data.length > 0)
	{
		*val = (bool)read_data.data[0];
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_short(int fd, const char* address, short* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 1, &read_data);
	if (ret && read_data.length > 0)
	{
		*val = bytes2short(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_ushort(int fd, const char* address, ushort* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 1, &read_data);
	if (ret && read_data.length >= 2)
	{
		*val = bytes2ushort(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_int32(int fd, const char* address, int32* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret && read_data.length >= 4)
	{
		*val = bytes2int32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_uint32(int fd, const char* address, uint32* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret && read_data.length >= 2)
	{
		*val = bytes2uint32(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_int64(int fd, const char* address, int64* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret && read_data.length >= 8)
	{
		*val = bytes2bigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_uint64(int fd, const char* address, uint64* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret && read_data.length >= 8)
	{
		*val = bytes2ubigInt(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_float(int fd, const char* address, float* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 2, &read_data);
	if (ret && read_data.length >= 4)
	{
		*val = bytes2float(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_double(int fd, const char* address, double* val)
{
	bool ret = false;
	byte_array_info read_data;
	memset(&read_data, 0, sizeof(read_data));
	ret = read_word_value(fd, address, 4, &read_data);
	if (ret && read_data.length >= 8)
	{
		*val = bytes2double(read_data.data);
		RELEASE_DATA(read_data.data);
	}
	return ret;
}

bool mc_read_string(int fd, const char* address, int length, char** val)
{
	bool ret = false;
	if (length > 0)
	{
		byte_array_info read_data;
		memset(&read_data, 0, sizeof(read_data));
		int read_len = (length % 2) == 1 ? length + 1 : length;
		ret = read_word_value(fd, address, length / 2, &read_data);
		if (ret && read_data.length >= read_len)
		{
			char* ret_str = (char*)malloc(read_len);
			memset(ret_str, 0, read_len);
			memcpy(ret_str, read_data.data, read_len);
			RELEASE_DATA(read_data.data);
			*val = ret_str;
		}
	}
	return ret;
}

bool mc_write_bool(int fd, const char* address, bool val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 1;
		bool_array_info write_data;
		bool* data = (bool*)malloc(write_len);
		data[0] = val;

		write_data.data = data;
		write_data.length = write_len;
		ret = write_bool_value(fd, address, 1, write_data);
	}
	return ret;
}

bool mc_write_short(int fd, const char* address, short val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 2;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		short2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 1, write_data);
	}
	return ret;
}

bool mc_write_ushort(int fd, const char* address, ushort val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 2;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		ushort2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 1, write_data);
	}
	return ret;
}

bool mc_write_int32(int fd, const char* address, int32 val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 4;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		int2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 2, write_data);
	}
	return ret;
}

bool mc_write_uint32(int fd, const char* address, uint32 val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 4;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		uint2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 2, write_data);
	}
	return ret;
}

bool mc_write_int64(int fd, const char* address, int64 val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 8;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		bigInt2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 4, write_data);
	}
	return ret;
}

bool mc_write_uint64(int fd, const char* address, uint64 val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 8;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		ubigInt2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 4, write_data);
	}
	return ret;
}

bool mc_write_float(int fd, const char* address, float val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 4;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		float2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 2, write_data);
	}
	return ret;
}

bool mc_write_double(int fd, const char* address, double val)
{
	bool ret = false;
	if (fd > 0 && address != NULL)
	{
		int write_len = 8;
		byte_array_info write_data;
		memset(&write_data, 0, sizeof(write_data));
		write_data.data = (byte*)malloc(write_len);
		write_data.length = write_len;

		double2bytes(val, write_data.data);
		ret = write_word_value(fd, address, 4, write_data);
	}
	return ret;
}

bool mc_write_string(int fd, const char* address, int length, const char* val)
{
	bool ret = false;
	if (fd > 0 && address != NULL && val != NULL)
	{
		int write_len = (length % 2) == 1 ? length + 1 : length;
		byte_array_info write_data;
		write_data.data = (byte*)malloc(write_len);
		memset(write_data.data, 0, write_len);
		memcpy(write_data.data, val, length);
		write_data.length = write_len;

		ret = write_word_value(fd, address, write_len / 2, write_data);
	}
	return ret;
}