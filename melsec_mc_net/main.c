#ifdef _WIN32
#include <WinSock2.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable : 4996)

#define GET_RESULT(ret)     \
	{                       \
		if (!ret)           \
			failed_count++; \
	}

// #define USE_SO
#ifndef USE_SO
#include "melsec_mc_bin.h"
#include "melsec_mc_ascii.h"
#endif

#ifdef USE_SO
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <memory.h>
#include <dlfcn.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

#include "typedef.h"

typedef bool (*pmc_mc_connect)(char* ip_addr, int port, byte network_addr, byte station_addr);
typedef bool (*pmc_mc_disconnect)(int fd);

typedef bool (*pmc_write_bool)(int fd, const char* address, bool val); // write
typedef bool (*pmc_write_short)(int fd, const char* address, short val);
typedef bool (*pmc_write_ushort)(int fd, const char* address, ushort val);
typedef bool (*pmc_write_int32)(int fd, const char* address, int32 val);
typedef bool (*pmc_write_uint32)(int fd, const char* address, uint32 val);
typedef bool (*pmc_write_int64)(int fd, const char* address, int64 val);
typedef bool (*pmc_write_uint64)(int fd, const char* address, uint64 val);
typedef bool (*pmc_write_float)(int fd, const char* address, float val);
typedef bool (*pmc_write_double)(int fd, const char* address, double val);
typedef bool (*pmc_write_string)(int fd, const char* address, int length, const char* val);

typedef bool (*pmc_read_bool)(int fd, const char* address, bool* val); // read
typedef bool (*pmc_read_short)(int fd, const char* address, short* val);
typedef bool (*pmc_read_ushort)(int fd, const char* address, ushort* val);
typedef bool (*pmc_read_int32)(int fd, const char* address, int32* val);
typedef bool (*pmc_read_uint32)(int fd, const char* address, uint32* val);
typedef bool (*pmc_read_int64)(int fd, const char* address, int64* val);
typedef bool (*pmc_read_uint64)(int fd, const char* address, uint64* val);
typedef bool (*pmc_read_float)(int fd, const char* address, float* val);
typedef bool (*pmc_read_double)(int fd, const char* address, double* val);
typedef bool (*pmc_read_string)(int fd, const char* address, int length, const char* val);

pmc_mc_connect mc_connect;
pmc_mc_disconnect mc_disconnect;

pmc_write_bool mc_write_bool;
pmc_write_short mc_write_short;
pmc_write_ushort mc_write_ushort;
pmc_write_int32 mc_write_int32;
pmc_write_uint32 mc_write_uint32;
pmc_write_int64 mc_write_int64;
pmc_write_uint64 mc_write_uint64;
pmc_write_float mc_write_float;
pmc_write_double mc_write_double;
pmc_write_string mc_write_string;

pmc_read_bool mc_read_bool;
pmc_read_short mc_read_short;
pmc_read_ushort mc_read_ushort;
pmc_read_int32 mc_read_int32;
pmc_read_uint32 mc_read_uint32;
pmc_read_int64 mc_read_int64;
pmc_read_uint64 mc_read_uint64;
pmc_read_float mc_read_float;
pmc_read_double mc_read_double;
pmc_read_string mc_read_string;

void libso_fun(char* szdllpath)
{
	void* handle_so;
	handle_so = dlopen(szdllpath, RTLD_NOW);
	if (!handle_so)
	{
		printf("%s\n", dlerror());
	}

	mc_connect = (pmc_mc_connect)dlsym(handle_so, "mc_connect");
	mc_disconnect = (pmc_mc_disconnect)dlsym(handle_so, "mc_disconnect");

	mc_write_bool = (pmc_write_bool)dlsym(handle_so, "mc_write_bool");
	mc_write_short = (pmc_write_short)dlsym(handle_so, "mc_write_short");
	mc_write_ushort = (pmc_write_ushort)dlsym(handle_so, "mc_write_ushort");
	mc_write_int32 = (pmc_write_int32)dlsym(handle_so, "mc_write_int32");
	mc_write_uint32 = (pmc_write_uint32)dlsym(handle_so, "mc_write_uint32");
	mc_write_int64 = (pmc_write_int64)dlsym(handle_so, "mc_write_int64");
	mc_write_uint64 = (pmc_write_uint64)dlsym(handle_so, "mc_write_uint64");
	mc_write_float = (pmc_write_float)dlsym(handle_so, "mc_write_float");
	mc_write_double = (pmc_write_double)dlsym(handle_so, "mc_write_double");
	mc_write_string = (pmc_write_string)dlsym(handle_so, "mc_write_string");

	mc_read_bool = (pmc_read_bool)dlsym(handle_so, "mc_read_bool");
	mc_read_short = (pmc_read_short)dlsym(handle_so, "mc_read_short");
	mc_read_ushort = (pmc_read_ushort)dlsym(handle_so, "mc_read_ushort");
	mc_read_int32 = (pmc_read_int32)dlsym(handle_so, "mc_read_int32");
	mc_read_uint32 = (pmc_read_uint32)dlsym(handle_so, "mc_read_uint32");
	mc_read_int64 = (pmc_read_int64)dlsym(handle_so, "mc_read_int64");
	mc_read_uint64 = (pmc_read_uint64)dlsym(handle_so, "mc_read_uint64");
	mc_read_float = (pmc_read_float)dlsym(handle_so, "mc_read_float");
	mc_read_double = (pmc_read_double)dlsym(handle_so, "mc_read_double");
	mc_read_string = (pmc_read_string)dlsym(handle_so, "mc_read_string");
	return;
}
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}
#endif

#ifdef USE_SO
	char szdllpath[1024];
	strcpy(szdllpath, "../melsec_mc_net.so");
	libso_fun(szdllpath);
#endif

	char* plc_ip = "127.0.0.1";
	int plc_port = 6001;
	if (argc > 1)
	{
		plc_ip = argv[1];
		plc_port = atoi(argv[2]);
	}
	int fd = mc_connect(plc_ip, plc_port, 0, 0);
	if (fd < 0)
		goto label_end;

	mc_error_code_e ret = MC_ERROR_CODE_FAILED;

#if false
	char* type = NULL;
	ret = mc_read_plc_type(fd, &type);
	printf("plc type: %s\n", type);
	RELEASE_DATA(type);
#endif

	const int TEST_COUNT = 5000;
	const int TEST_SLEEP_TIME = 200;
	int failed_count = 0;

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
		ret = mc_write_int32(fd, "D100", w_i_val);
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
		RELEASE_DATA(str_val);
		GET_RESULT(ret);

#ifdef _WIN32
		Sleep(TEST_SLEEP_TIME);
#else
		sleep(TEST_SLEEP_TIME);
#endif
	}

	printf("All Failed count: %d\n", failed_count);

	// mc_remote_run(fd);
	// mc_remote_stop(fd);
	// mc_remote_reset(fd);
	mc_disconnect(fd);

label_end:
#ifdef _WIN32
	WSACleanup();
#else
	;
#endif
}