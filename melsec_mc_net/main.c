#include "melsec_mc_bin.h"
#include "melsec_mc_ascii.h"
#ifdef WIN32
#include <WinSock2.h>
#endif
#include <stdio.h>
#pragma warning( disable : 4996)

#define GET_RESULT(ret){ if(!ret) faild_count++; }

int main(int argc, char** argv)
{
#ifdef WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}
#endif

	char* plc_ip = "192.168.0.235";
	int plc_port = 5002;
	if (argc > 1)
	{
		plc_ip = argv[1];
		plc_port = atoi(argv[2]);
	}
	int fd = mc_connect(plc_ip, plc_port, 0, 0);
	if (fd > 0)
	{
		bool ret = false;

		char* type = mc_read_plc_type(fd);
		printf("plc type: %s\n", type);
		free(type);

		const int TEST_COUNT = 5000;
		const int TEST_SLEEP_TIME = 200;
		int faild_count = 0;

		for (int i = 0; i < TEST_COUNT; i++)
		{
			printf("==============Test count: %d==============\n", i + 1);
			bool all_success = false;
			//////////////////////////////////////////////////////////////////////////
			bool val = true;
			ret = mc_write_bool(fd, "X1", val);
			printf("Write\t X1 \tbool:\t %d, \tret: %d\n", val, ret);
			GET_RESULT(ret);

			val = false;
			ret = mc_read_bool(fd, "x1", &val);
			printf("Read\t X1 \tbool:\t %d\n", val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			short w_s_val = 23;
			ret = mc_write_short(fd, "D100", w_s_val);
			printf("Write\t D100 \tshort:\t %d, \tret: %d\n", w_s_val, ret);
			GET_RESULT(ret);

			short s_val = 0;
			ret = mc_read_short(fd, "D100", &s_val);
			printf("Read\t D100 \tshort:\t %d\n", s_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			ushort w_us_val = 255;
			ret = mc_write_ushort(fd, "D100", w_us_val);
			printf("Write\t D100 \tushort:\t %d, \tret: %d\n", w_us_val, ret);
			GET_RESULT(ret);

			ushort us_val = 0;
			ret = mc_read_ushort(fd, "D100", &us_val);
			printf("Read\t D100 \tushort:\t %d\n", us_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			int32 w_i_val = 12345;
			ret = mc_write_int32(fd, "D0", w_i_val);
			printf("Write\t D100 \tint32:\t %d, \tret: %d\n", w_i_val, ret);
			GET_RESULT(ret);

			int i_val = 0;
			ret = mc_read_int32(fd, "D100", &i_val);
			printf("Read\t D100 \tint32:\t %d\n", i_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			uint32 w_ui_val = 22345;
			ret = mc_write_uint32(fd, "D100", w_ui_val);
			printf("Write\t D100 \tuint32:\t %d, \tret: %d\n", w_ui_val, ret);
			GET_RESULT(ret);

			uint32 ui_val = 0;
			ret = mc_read_uint32(fd, "D100", &ui_val);
			printf("Read\t D100 \tuint32:\t %d\n", ui_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			int64 w_i64_val = 333334554;
			ret = mc_write_int64(fd, "D100", w_i64_val);
			printf("Write\t D100 \tuint64:\t %lld, \tret: %d\n", w_i64_val, ret);
			GET_RESULT(ret);

			int64 i64_val = 0;
			ret = mc_read_int64(fd, "D100", &i64_val);
			printf("Read\t D100 \tint64:\t %lld\n", i64_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			uint64 w_ui64_val = 4333334554;
			ret = mc_write_uint64(fd, "D100", w_ui64_val);
			printf("Write\t D100 \tuint64:\t %lld, \tret: %d\n", w_ui64_val, ret);
			GET_RESULT(ret);

			int64 ui64_val = 0;
			ret = mc_read_uint64(fd, "D100", &ui64_val);
			printf("Read\t D100 \tuint64:\t %lld\n", ui64_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			float w_f_val = 32.454f;
			ret = mc_write_float(fd, "D100", w_f_val);
			printf("Write\t D100 \tfloat:\t %f, \tret: %d\n", w_f_val, ret);
			GET_RESULT(ret);

			float f_val = 0;
			ret = mc_read_float(fd, "D100", &f_val);
			printf("Read\t D100 \tfloat:\t %f\n", f_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			double w_d_val = 12345.6789;
			ret = mc_write_double(fd, "D100", w_d_val);
			printf("Write\t D100 \tdouble:\t %lf, \tret: %d\n", w_d_val, ret);
			GET_RESULT(ret);

			double d_val = 0;
			ret = mc_read_double(fd, "D100", &d_val);
			printf("Read\t D100 \tdouble:\t %lf\n", d_val);
			GET_RESULT(ret);

			//////////////////////////////////////////////////////////////////////////
			const char sz_write[] = "wqliceman@gmail.com";
			int length = sizeof(sz_write) / sizeof(sz_write[0]);
			ret = mc_write_string(fd, "D100", length, sz_write);
			printf("Write\t D100 \tstring:\t %s, \tret: %d\n", sz_write, ret);
			GET_RESULT(ret);

			char* str_val = NULL;
			ret = mc_read_string(fd, "D100", length, &str_val);
			printf("Read\t D100 \tstring:\t %s\n", str_val);
			free(str_val);
			GET_RESULT(ret);

			//Sleep(TEST_SLEEP_TIME);
		}

		printf("All Failed count: %d\n", faild_count);


		//mc_remote_run(fd);
		//mc_remote_stop(fd);
		//mc_remote_reset(fd);
		//mc_disconnect(fd);
	}

#ifdef WIN32
	WSACleanup();
#endif
}
