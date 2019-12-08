#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "melsec_helper.h"

// 从三菱地址，是否位读取进行创建读取的MC的核心报文
// is_bit 是否进行了位读取操作
byte_array_info build_read_core_command(melsec_mc_address_data address_data, bool is_bit)
{
	byte* command = (byte*)malloc(10);

	command[0] = 0x01;                                                      // 批量读取数据命令
	command[1] = 0x04;
	command[2] = is_bit ? (byte)0x01 : (byte)0x00;                           // 以点为单位还是字为单位成批读取
	command[3] = 0x00;
	command[4] = (byte)(address_data.address_start % 256);				// 起始地址的地位
	command[5] = (byte)(address_data.address_start >> 8);
	command[6] = (byte)(address_data.address_start >> 16);
	command[7] = address_data.data_type.data_code;                           // 指明读取的数据
	command[8] = (byte)(address_data.length % 256);                          // 软元件的长度
	command[9] = (byte)(address_data.length >> 8);

	byte_array_info ret;
	ret.data = command;
	ret.length = 10;
	return ret;
}

// 从三菱地址，是否位读取进行创建读取Ascii格式的MC的核心报文
// 是否进行了位读取操作
byte_array_info build_ascii_read_core_command(melsec_mc_address_data address_data, bool is_bit)
{
	byte* command = (byte*)malloc(20);

	command[0] = 0x30;                                    // 批量读取数据命令
	command[1] = 0x34;
	command[2] = 0x30;
	command[3] = 0x31;
	command[4] = 0x30;                                   // 以点为单位还是字为单位成批读取
	command[5] = 0x30;
	command[6] = 0x30;
	command[7] = is_bit ? (byte)0x31 : (byte)0x30;
	command[8] = (byte)(address_data.data_type.ascii_code[0]);          // 软元件类型
	command[9] = (byte)(address_data.data_type.ascii_code[1]);

	byte_array_info temp = build_bytes_from_address(address_data.address_start, address_data.data_type);
	command[10] = temp.data[0];            // 起始地址的地位
	command[11] = temp.data[1];
	command[12] = temp.data[2];
	command[13] = temp.data[3];
	command[14] = temp.data[4];
	command[15] = temp.data[5];
	free(temp.data);

	temp = build_ascii_bytes_from_ushort(address_data.length);
	command[16] = temp.data[0];                               // 软元件点数
	command[17] = temp.data[1];
	command[18] = temp.data[2];
	command[19] = temp.data[3];
	free(temp.data);

	byte_array_info ret;
	ret.data = command;
	ret.length = 20;
	return ret;
}

byte_array_info build_write_word_core_command(melsec_mc_address_data address_data, byte_array_info value)
{
	int val_len = 0;
	if (value.data != NULL)
		val_len = value.length;

	byte* command = (byte*)malloc(10 + val_len);
	command[0] = 0x01;											// 批量写入数据命令
	command[1] = 0x14;
	command[2] = 0x00;											// 以字为单位成批读取
	command[3] = 0x00;
	command[4] = (byte)(address_data.address_start % 256);		// 起始地址的地位
	command[5] = (byte)(address_data.address_start >> 8);
	command[6] = (byte)(address_data.address_start >> 16);
	command[7] = address_data.data_type.data_code;				// 指明写入的数据
	command[8] = (byte)((val_len >> 1) % 256);					// 软元件长度的地位
	command[9] = (byte)((val_len >> 1) >> 8);
	if (value.data != NULL)
	{
		memcpy(command + 10, value.data, val_len);
		free(value.data);
	}

	byte_array_info ret;
	ret.data = command;
	ret.length = 10 + val_len;
	return ret;
}

byte_array_info build_ascii_write_word_core_command(melsec_mc_address_data address_data, byte_array_info value)
{
	int val_len = 0;
	if (value.data != NULL) val_len = value.length;

	byte_array_info buffer = build_ascii_bytes_from_byte_array(value.data, val_len);
	if (buffer.data != NULL)
		val_len = buffer.length;

	byte* command = (byte*)malloc(20 + val_len);
	command[0] = 0x31;                                  // 批量写入的命令
	command[1] = 0x34;
	command[2] = 0x30;
	command[3] = 0x31;
	command[4] = 0x30;                                 // 子命令
	command[5] = 0x30;
	command[6] = 0x30;
	command[7] = 0x30;
	command[8] = (byte)address_data.data_type.ascii_code[0]; // 软元件类型
	command[9] = (byte)address_data.data_type.ascii_code[1];

	byte_array_info temp = build_bytes_from_address(address_data.address_start, address_data.data_type);
	command[10] = temp.data[0];            // 起始地址的地位
	command[11] = temp.data[1];
	command[12] = temp.data[2];
	command[13] = temp.data[3];
	command[14] = temp.data[4];
	command[15] = temp.data[5];
	free(temp.data);

	temp = build_ascii_bytes_from_ushort(address_data.length);
	command[16] = temp.data[0];                               // 软元件点数
	command[17] = temp.data[1];
	command[18] = temp.data[2];
	command[19] = temp.data[3];
	free(temp.data);

	if (value.data != NULL)
		memcpy(command + 20, buffer.data, val_len);
	free(buffer.data);

	byte_array_info ret;
	ret.data = command;
	ret.length = 20 + val_len;
	return ret;
}

byte_array_info build_write_bit_core_command(melsec_mc_address_data address_data, bool_array_info value)
{
	int val_len = 0;
	if (value.data != NULL) val_len = value.length;

	byte_array_info buffer = trans_bool_array_to_byte_data(value);
	val_len = buffer.length;

	byte* command = (byte*)malloc(10 + val_len);
	command[0] = 0x01;                                       // 批量写入数据命令
	command[1] = 0x14;
	command[2] = 0x01;                                       // 以位为单位成批写入
	command[3] = 0x00;
	command[4] = (byte)(address_data.address_start % 256);   // 起始地址的地位
	command[5] = (byte)(address_data.address_start >> 8);
	command[6] = (byte)(address_data.address_start >> 16);
	command[7] = address_data.data_type.data_code;            // 指明读取的数据
	command[8] = (byte)(address_data.length % 256);           // 软元件的长度
	command[9] = (byte)(address_data.length >> 8);

	memcpy(command + 10, buffer.data, val_len);
	free(buffer.data);

	byte_array_info ret;
	ret.data = command;
	ret.length = 10 + val_len;
	return ret;
}

byte_array_info build_ascii_write_bit_core_command(melsec_mc_address_data address_data, bool_array_info value)
{
	int val_len = 0;
	if (value.data != NULL) val_len = value.length;

	byte_array_info buffer = build_ascii_bytes_from_bool_array(value.data, val_len);
	if (buffer.data != NULL)
		val_len = buffer.length;

	byte* command = (byte*)malloc(20 + val_len);
	command[0] = 0x31;                                                                              // 批量写入的命令
	command[1] = 0x34;
	command[2] = 0x30;
	command[3] = 0x31;
	command[4] = 0x30;                                                                              // 子命令
	command[5] = 0x30;
	command[6] = 0x30;
	command[7] = 0x31;
	command[8] = (byte)address_data.data_type.ascii_code[0]; // 软元件类型
	command[9] = (byte)address_data.data_type.ascii_code[1];
	byte_array_info temp = build_bytes_from_address(address_data.address_start, address_data.data_type);
	command[10] = temp.data[0];            // 起始地址的地位
	command[11] = temp.data[1];
	command[12] = temp.data[2];
	command[13] = temp.data[3];
	command[14] = temp.data[4];
	command[15] = temp.data[5];
	free(temp.data);

	temp = build_ascii_bytes_from_ushort(address_data.length);
	command[16] = temp.data[0];                               // 软元件点数
	command[17] = temp.data[1];
	command[18] = temp.data[2];
	command[19] = temp.data[3];
	free(temp.data);

	if (value.data != NULL)
		memcpy(command + 20, buffer.data, val_len);

	free(buffer.data);

	byte_array_info ret;
	ret.data = command;
	ret.length = 20 + val_len;
	return ret;
}

byte_array_info build_bytes_from_address(int address, melse_mc_data_type type)
{
	byte* out = NULL;
	char buffer[10];
	memset(buffer, 0, 10);

#ifdef _WIN32
	_itoa(address, buffer, type.from_base);
#else
	itoa(address, buffer, type.from_base);
#endif
	address = atoi(buffer);
	return build_ascii_bytes_from_int(address);
}

byte_array_info build_ascii_bytes_from_ushort(unsigned short data)
{
	byte* out = NULL;
	int length = 2;
	char hex_str[] = "0123456789ABCDEF";

	out = (byte*)malloc(length * 2 + 1);
	memset((void*)out, 0, length * 2 + 1);

	byte temp[2];
	temp[0] = (byte)(0xff & data);
	temp[1] = (byte)(0xff & (data >> 8));

	for (int i = 0; i < length; i++) {
		(out)[i * 2 + 0] = hex_str[(temp[i] >> 4) & 0x0F];
		(out)[i * 2 + 1] = hex_str[(temp[i]) & 0x0F];
	}
	byte_array_info ret;
	ret.data = out;
	ret.length = length * 2;
	return ret;
}

byte_array_info build_ascii_bytes_from_int(int data)
{
	byte* out = NULL;
	int length = 4;

	char hex_str[] = "0123456789ABCDEF";

	out = (byte*)malloc(length * 2 + 1);
	memset((void*)out, 0, length * 2 + 1);

	byte temp[4];
	temp[0] = (byte)(0xff & data);
	temp[1] = (byte)(0xff & (data >> 8));
	temp[2] = (byte)(0xff & (data >> 16));
	temp[3] = (byte)(0xff & (data >> 24));

	for (int i = 0; i < length; i++) {
		(out)[i * 2 + 0] = hex_str[(temp[i] >> 4) & 0x0F];
		(out)[i * 2 + 1] = hex_str[(temp[i]) & 0x0F];
	}

	byte_array_info ret;
	ret.data = out;
	ret.length = 8;
	return ret;
}

byte_array_info build_ascii_bytes_from_byte_array(const byte* data, int length)
{
	byte* out = NULL;
	if (data && length > 0)
	{
		char hex_str[] = "0123456789ABCDEF";

		out = (byte*)malloc(length * 2 + 1);
		memset((void*)out, 0, length * 2 + 1);

		for (int i = 0; i < length; i++) {
			(out)[i * 2 + 0] = hex_str[(data[i] >> 4) & 0x0F];
			(out)[i * 2 + 1] = hex_str[(data[i]) & 0x0F];
		}
	}
	byte_array_info ret;
	ret.data = out;
	ret.length = length * 2;
	return ret;
}

byte_array_info build_ascii_bytes_from_bool_array(const bool* value, int length)
{
	byte* out = NULL;
	if (value != NULL && length > 0)
	{
		out = (byte*)malloc(length);
		for (int i = 0; i < length; i++)
		{
			out[i] = value[i] ? (byte)0x31 : (byte)0x30;
		}
	}

	byte_array_info ret;
	ret.data = out;
	ret.length = length;
	return ret;
}

byte_array_info trans_bool_array_to_byte_data(bool_array_info value)
{
	byte* out = NULL;
	int length = 0;
	if (value.data != NULL)
	{
		length = (value.length + 1) / 2;
		out = (byte*)malloc(length);
		memset(out, 0, length);

		for (int i = 0; i < length; i++)
		{
			if (value.data[i * 2 + 0])
				out[i] += 0x10;

			if ((i * 2 + 1) < length)
			{
				if (value.data[i * 2 + 1])
					out[i] += 0x01;
			}
		}
	}
	byte_array_info ret;
	ret.data = out;
	ret.length = length;
	return ret;
}

byte_array_info calculate_CRC(byte_array_info data)
{
	int sum = 0;
	int data_len = data.length;
	for (int i = 1; i < data_len - 2; i++)
	{
		sum += data.data[i];
	}
	return build_ascii_bytes_from_ushort((unsigned short)sum);
}

bool check_CRC(byte_array_info arr_data)
{
	bool ret = false;
	if (arr_data.data)
	{
		byte_array_info crc = calculate_CRC(arr_data);
		int data_len = arr_data.length;
		if ((crc.data[0] == arr_data.data[data_len - 2]) && (crc.data[1] != arr_data.data[data_len - 1]))
			ret = true;
	}
	return ret;
}

bool mc_parse_read_response(byte_array_info response, byte_array_info* data)
{
	bool ret = false;
	if (response.length == 0)
		return ret;

	int min = 11;
	byte count[2];
	count[0] = response.data[min - 3];
	count[1] = response.data[min - 4];
	byte code[2];
	code[0] = response.data[min - 2];
	code[1] = response.data[min - 1];

	uint16_t rsCount = count[0] * 256 + count[1] - 2;
	uint16_t rsCode = code[0] * 256 + code[1];
	ret = rsCode == 0 && rsCount == (response.length - min);

	if (rsCount > 0 && (data != NULL))
	{
		data->data = (byte*)malloc(rsCount);
		memset(data->data, 0, rsCount);
		memcpy(data->data, response.data + 11, rsCount);
		data->length = rsCount;
	}
	return ret;
}

bool mc_parse_write_response(byte_array_info response, byte_array_info* data)
{
	bool ret = false;
	if (response.length == 0)
		return ret;

	int min = 11;
	byte count[2];
	count[0] = response.data[min - 3];
	count[1] = response.data[min - 4];
	byte code[2];
	code[0] = response.data[min - 2];
	code[1] = response.data[min - 1];

	uint16_t rsCount = count[0] * 256 + count[1] - 2;
	uint16_t rsCode = code[0] * 256 + code[1];
	ret = rsCode == 0 && rsCount == (response.length - min);

	//code 以后的内容返回
	if (rsCount > 2 && (data != NULL))
	{
		data->data = (byte*)malloc(rsCount);
		memset(data->data, 0, rsCount);
		memcpy(data->data, response.data + 11, rsCount);
		data->length = rsCount;
	}
	return ret;
}