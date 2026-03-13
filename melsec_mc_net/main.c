#ifdef _WIN32
#include <WinSock2.h>
#else
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "melsec_mc_ascii.h"
#include "melsec_mc_bin.h"
#include "network_init.h"

typedef int (*mc_connect_fn_t)(char* ip_addr, int port, byte network_addr, byte station_addr);
typedef bool (*mc_disconnect_fn_t)(int fd);
typedef mc_error_code_e (*mc_write_int32_fn_t)(int fd, const char* address, int32 val);
typedef mc_error_code_e (*mc_read_int32_fn_t)(int fd, const char* address, int32* val);
typedef mc_error_code_e (*mc_write_bool_fn_t)(int fd, const char* address, bool val);
typedef mc_error_code_e (*mc_read_bool_fn_t)(int fd, const char* address, bool* val);

static mc_connect_fn_t g_stress_connect = NULL;
static mc_disconnect_fn_t g_stress_disconnect = NULL;
static mc_write_int32_fn_t g_stress_write_int32 = NULL;
static mc_read_int32_fn_t g_stress_read_int32 = NULL;
static mc_write_bool_fn_t g_stress_write_bool = NULL;
static mc_read_bool_fn_t g_stress_read_bool = NULL;

static void configure_stress_protocol(bool use_ascii)
{
    if (use_ascii) {
        g_stress_connect = mc_ascii_connect;
        g_stress_disconnect = mc_ascii_disconnect;
        g_stress_write_int32 = mc_ascii_write_int32;
        g_stress_read_int32 = mc_ascii_read_int32;
        g_stress_write_bool = mc_ascii_write_bool;
        g_stress_read_bool = mc_ascii_read_bool;
    }
    else {
        g_stress_connect = mc_connect;
        g_stress_disconnect = mc_disconnect;
        g_stress_write_int32 = mc_write_int32;
        g_stress_read_int32 = mc_read_int32;
        g_stress_write_bool = mc_write_bool;
        g_stress_read_bool = mc_read_bool;
    }
}

static void print_ascii_case_ret(const char* name, mc_error_code_e ret)
{
    printf("[ascii] %-24s ret=%d\n", name, (int)ret);
}

static void print_ascii_cmp_bool(const char* tag, bool expected, bool actual)
{
    printf("[ascii][cmp] %-18s expected=%d actual=%d\n", tag, expected ? 1 : 0, actual ? 1 : 0);
}

static void print_ascii_cmp_i32(const char* tag, int32 expected, int32 actual)
{
    printf("[ascii][cmp] %-18s expected=%d actual=%d\n", tag, expected, actual);
}

static void print_ascii_cmp_u32(const char* tag, uint32 expected, uint32 actual)
{
    printf("[ascii][cmp] %-18s expected=%u actual=%u\n", tag, expected, actual);
}

static void print_ascii_cmp_i64(const char* tag, int64 expected, int64 actual)
{
    printf("[ascii][cmp] %-18s expected=%lld actual=%lld\n", tag, expected, actual);
}

static void print_ascii_cmp_u64(const char* tag, uint64 expected, uint64 actual)
{
    printf("[ascii][cmp] %-18s expected=%llu actual=%llu\n", tag, expected, actual);
}

static void print_ascii_cmp_float(const char* tag, float expected, float actual)
{
    union { float f; uint32 u; } e = { 0 }, a = { 0 };
    e.f = expected;
    a.f = actual;
    printf("[ascii][cmp] %-18s expected=%f(0x%08X) actual=%f(0x%08X)\n", tag, expected, e.u, actual, a.u);
}

static void print_ascii_cmp_double(const char* tag, double expected, double actual)
{
    union { double d; uint64 u; } e = { 0 }, a = { 0 };
    e.d = expected;
    a.d = actual;
    printf("[ascii][cmp] %-18s expected=%.10f(0x%016llX) actual=%.10f(0x%016llX)\n", tag, expected, e.u, actual, a.u);
}

static void print_ascii_cmp_string(const char* tag, const char* expected, const char* actual)
{
    printf("[ascii][cmp] %-18s expected=\"%s\" actual=\"%s\"\n", tag,
        expected != NULL ? expected : "<null>",
        actual != NULL ? actual : "<null>");
}

static int run_ascii_api_smoke(const char* ip, int port)
{
    int fd = mc_ascii_connect((char*)ip, port, 0, 0);
    if (fd <= 0) {
        printf("[ascii] connect failed: %s:%d\n", ip, port);
        return -1;
    }

    int strict_fail = 0;
    mc_error_code_e ret;

    bool wb = true;
    bool rb = false;
    ret = mc_ascii_write_bool(fd, "M100", wb);
    print_ascii_case_ret("write_bool", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_bool(fd, "M100", &rb);
        print_ascii_case_ret("read_bool", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_bool("bool(M100)", wb, rb);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || rb != wb) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    short ws = (short)-1234;
    short rs = 0;
    ret = mc_ascii_write_short(fd, "D100", ws);
    print_ascii_case_ret("write_short", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_short(fd, "D100", &rs);
        print_ascii_case_ret("read_short", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_i32("short(D100)", ws, rs);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || rs != ws) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    ushort wus = (ushort)54321;
    ushort rus = 0;
    ret = mc_ascii_write_ushort(fd, "D101", wus);
    print_ascii_case_ret("write_ushort", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_ushort(fd, "D101", &rus);
        print_ascii_case_ret("read_ushort", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_u32("ushort(D101)", wus, rus);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || rus != wus) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    int32 wi32 = (int32)-123456789;
    int32 ri32 = 0;
    ret = mc_ascii_write_int32(fd, "D110", wi32);
    print_ascii_case_ret("write_int32", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_int32(fd, "D110", &ri32);
        print_ascii_case_ret("read_int32", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_i32("int32(D110)", wi32, ri32);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || ri32 != wi32) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    uint32 wu32 = (uint32)3234567890U;
    uint32 ru32 = 0;
    ret = mc_ascii_write_uint32(fd, "D112", wu32);
    print_ascii_case_ret("write_uint32", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_uint32(fd, "D112", &ru32);
        print_ascii_case_ret("read_uint32", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_u32("uint32(D112)", wu32, ru32);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || ru32 != wu32) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    int64 wi64 = (int64)-1234567890123LL;
    int64 ri64 = 0;
    ret = mc_ascii_write_int64(fd, "D120", wi64);
    print_ascii_case_ret("write_int64", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_int64(fd, "D120", &ri64);
        print_ascii_case_ret("read_int64", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_i64("int64(D120)", wi64, ri64);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || ri64 != wi64) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    uint64 wu64 = (uint64)12345678901234ULL;
    uint64 ru64 = 0;
    ret = mc_ascii_write_uint64(fd, "D124", wu64);
    print_ascii_case_ret("write_uint64", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_uint64(fd, "D124", &ru64);
        print_ascii_case_ret("read_uint64", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_u64("uint64(D124)", wu64, ru64);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || ru64 != wu64) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    float wf = 12.375f;
    float rf = 0.0f;
    ret = mc_ascii_write_float(fd, "D130", wf);
    print_ascii_case_ret("write_float", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_float(fd, "D130", &rf);
        print_ascii_case_ret("read_float", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_float("float(D130)", wf, rf);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || rf != wf) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    double wd = 1234.5678;
    double rd = 0.0;
    ret = mc_ascii_write_double(fd, "D140", wd);
    print_ascii_case_ret("write_double", ret);
    if (ret == MC_ERROR_CODE_SUCCESS) {
        ret = mc_ascii_read_double(fd, "D140", &rd);
        print_ascii_case_ret("read_double", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            print_ascii_cmp_double("double(D140)", wd, rd);
        }
        if (ret != MC_ERROR_CODE_SUCCESS || rd != wd) {
            strict_fail++;
        }
    }
    else {
        strict_fail++;
    }

    {
        const char* wstr = "ASCII_TEST";
        char* rstr = NULL;
        ret = mc_ascii_write_string(fd, "D150", (int)strlen(wstr), wstr);
        print_ascii_case_ret("write_string", ret);
        if (ret == MC_ERROR_CODE_SUCCESS) {
            ret = mc_ascii_read_string(fd, "D150", (int)strlen(wstr), &rstr);
            print_ascii_case_ret("read_string", ret);
            if (ret == MC_ERROR_CODE_SUCCESS) {
                print_ascii_cmp_string("string(D150)", wstr, rstr);
            }
            if (ret != MC_ERROR_CODE_SUCCESS || rstr == NULL || memcmp(rstr, wstr, strlen(wstr)) != 0) {
                strict_fail++;
            }
            RELEASE_DATA(rstr);
        }
        else {
            strict_fail++;
        }
    }

    // Remote control and model read are printed for protocol verification, not counted as strict failures.
    /*
    ret = mc_ascii_remote_run(fd);
    print_ascii_case_ret("remote_run", ret);
    ret = mc_ascii_remote_stop(fd);
    print_ascii_case_ret("remote_stop", ret);
    ret = mc_ascii_remote_reset(fd);
    print_ascii_case_ret("remote_reset", ret);

    {
        char* plc_type = NULL;
        ret = mc_ascii_read_plc_type(fd, &plc_type);
        print_ascii_case_ret("read_plc_type", ret);
        if (ret == MC_ERROR_CODE_SUCCESS && plc_type != NULL) {
            printf("[ascii] plc_type=%s\n", plc_type);
        }
        RELEASE_DATA(plc_type);
    }
    */

    mc_ascii_disconnect(fd);
    printf("[ascii] strict_fail=%d\n", strict_fail);
    return strict_fail;
}

typedef struct {
    int fd;
    int thread_id;
    int iterations;
    char d_addr[32];
    char m_addr[32];

    long api_calls;
    long checks;
    long mismatches;
    long io_errors;
} stress_worker_arg_t;

typedef struct {
    long api_calls;
    long checks;
    long mismatches;
    long io_errors;
    double elapsed_sec;
} stress_summary_t;

#ifdef _WIN32
typedef HANDLE mc_thread_t;
static DWORD WINAPI stress_worker_thread(LPVOID param)
#else
typedef pthread_t mc_thread_t;
static void* stress_worker_thread(void* param)
#endif
{
    stress_worker_arg_t* arg = (stress_worker_arg_t*)param;

    if (g_stress_write_int32 == NULL || g_stress_read_int32 == NULL ||
        g_stress_write_bool == NULL || g_stress_read_bool == NULL) {
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }

    for (int i = 0; i < arg->iterations; i++) {
        int32 write_d = (int32)((arg->thread_id << 20) ^ i);
        int32 read_d = 0;
        bool write_m = ((i & 1) != 0);
        bool read_m = false;

        mc_error_code_e ret = g_stress_write_int32(arg->fd, arg->d_addr, write_d);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        ret = g_stress_read_int32(arg->fd, arg->d_addr, &read_d);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        arg->checks++;
        if (read_d != write_d) {
            arg->mismatches++;
        }

        ret = g_stress_write_bool(arg->fd, arg->m_addr, write_m);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        ret = g_stress_read_bool(arg->fd, arg->m_addr, &read_m);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        arg->checks++;
        if (read_m != write_m) {
            arg->mismatches++;
        }
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static void run_sleep_ms(int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep((useconds_t)(ms * 1000));
#endif
}

static double now_seconds(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq;
    static BOOL initialized = FALSE;
    LARGE_INTEGER counter;

    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = TRUE;
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#endif
}

static int create_threads_and_wait(stress_worker_arg_t* args, int thread_count)
{
    mc_thread_t* threads = (mc_thread_t*)malloc((size_t)thread_count * sizeof(mc_thread_t));
    if (threads == NULL) {
        return -1;
    }

    int started = 0;
    for (int i = 0; i < thread_count; i++) {
#ifdef _WIN32
        threads[i] = CreateThread(NULL, 0, stress_worker_thread, &args[i], 0, NULL);
        if (threads[i] == NULL) {
            break;
        }
#else
        if (pthread_create(&threads[i], NULL, stress_worker_thread, &args[i]) != 0) {
            break;
        }
#endif
        started++;
    }

    for (int i = 0; i < started; i++) {
#ifdef _WIN32
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
#else
        pthread_join(threads[i], NULL);
#endif
    }

    free(threads);
    return (started == thread_count) ? 0 : -1;
}

static stress_summary_t summarize(const stress_worker_arg_t* args, int thread_count, double elapsed_sec)
{
    stress_summary_t s = { 0 };
    for (int i = 0; i < thread_count; i++) {
        s.api_calls += args[i].api_calls;
        s.checks += args[i].checks;
        s.mismatches += args[i].mismatches;
        s.io_errors += args[i].io_errors;
    }
    s.elapsed_sec = elapsed_sec;
    return s;
}

static void print_summary(const char* title, int thread_count, int iterations, const stress_summary_t* s)
{
    double mismatch_rate = (s->checks > 0) ? ((double)s->mismatches * 100.0 / (double)s->checks) : 0.0;
    double qps = (s->elapsed_sec > 0.0) ? ((double)s->api_calls / s->elapsed_sec) : 0.0;
    double check_rate = (s->elapsed_sec > 0.0) ? ((double)s->checks / s->elapsed_sec) : 0.0;

    printf("\n==== %s ====\n", title);
    printf("threads=%d, iterations/thread=%d\n", thread_count, iterations);
    printf("elapsed=%.3f sec\n", s->elapsed_sec);
    printf("api_calls=%ld, checks=%ld, io_errors=%ld\n", s->api_calls, s->checks, s->io_errors);
    printf("mismatches=%ld, mismatch_rate=%.6f%%\n", s->mismatches, mismatch_rate);
    printf("throughput_api=%.2f ops/s, throughput_checks=%.2f checks/s\n", qps, check_rate);
}

static int run_same_fd_stress(const char* ip, int port, int thread_count, int iterations)
{
    if (g_stress_connect == NULL || g_stress_disconnect == NULL) {
        return -1;
    }

    int fd = g_stress_connect((char*)ip, port, 0, 0);
    if (fd <= 0) {
        printf("[same-fd] connect failed: %s:%d\n", ip, port);
        return -1;
    }

    stress_worker_arg_t* args = (stress_worker_arg_t*)calloc((size_t)thread_count, sizeof(stress_worker_arg_t));
    if (args == NULL) {
        g_stress_disconnect(fd);
        return -1;
    }

    for (int i = 0; i < thread_count; i++) {
        args[i].fd = fd;
        args[i].thread_id = i;
        args[i].iterations = iterations;
        snprintf(args[i].d_addr, sizeof(args[i].d_addr), "D%d", 1000 + i * 2);
        snprintf(args[i].m_addr, sizeof(args[i].m_addr), "M%d", 1000 + i);
    }

    double start = now_seconds();
    int thread_ret = create_threads_and_wait(args, thread_count);
    double elapsed = now_seconds() - start;

    stress_summary_t s = summarize(args, thread_count, elapsed);
    print_summary("Same-FD Mixed R/W", thread_count, iterations, &s);

    free(args);
    g_stress_disconnect(fd);

    return thread_ret;
}

static int run_multi_fd_stress(const char* ip, int port, int thread_count, int iterations)
{
    if (g_stress_connect == NULL || g_stress_disconnect == NULL) {
        return -1;
    }

    int* fds = (int*)calloc((size_t)thread_count, sizeof(int));
    stress_worker_arg_t* args = (stress_worker_arg_t*)calloc((size_t)thread_count, sizeof(stress_worker_arg_t));
    if (fds == NULL || args == NULL) {
        free(fds);
        free(args);
        return -1;
    }

    for (int i = 0; i < thread_count; i++) {
        fds[i] = g_stress_connect((char*)ip, port, 0, 0);
        if (fds[i] <= 0) {
            printf("[multi-fd] connect failed at idx=%d: %s:%d\n", i, ip, port);
            for (int j = 0; j < i; j++) {
                if (fds[j] > 0) {
                    g_stress_disconnect(fds[j]);
                }
            }
            free(fds);
            free(args);
            return -1;
        }

        args[i].fd = fds[i];
        args[i].thread_id = i;
        args[i].iterations = iterations;
        snprintf(args[i].d_addr, sizeof(args[i].d_addr), "D%d", 3000 + i * 2);
        snprintf(args[i].m_addr, sizeof(args[i].m_addr), "M%d", 3000 + i);
    }

    double start = now_seconds();
    int thread_ret = create_threads_and_wait(args, thread_count);
    double elapsed = now_seconds() - start;

    stress_summary_t s = summarize(args, thread_count, elapsed);
    print_summary("Multi-FD Concurrent Throughput", thread_count, iterations, &s);

    for (int i = 0; i < thread_count; i++) {
        if (fds[i] > 0) {
            g_stress_disconnect(fds[i]);
        }
    }

    free(fds);
    free(args);
    return thread_ret;
}

int main(int argc, char** argv)
{
    // Test protocol selector: set to "bin" or "ascii".
    const char* test_protocol = "ascii";
    int use_ascii_protocol = (test_protocol[0] == 'a' || test_protocol[0] == 'A');

    if (mc_network_init() != MC_ERROR_CODE_SUCCESS) {
        printf("network init failed\n");
        return -1;
    }

    const char* plc_ip = "127.0.0.1";
    int plc_port = 6001;
    int same_fd_threads = 8;
    int same_fd_iterations = 2000;
    int multi_fd_threads = 8;
    int multi_fd_iterations = 2000;

    if (argc > 1) {
        plc_ip = argv[1];
    }
    if (argc > 2) {
        plc_port = atoi(argv[2]);
    }
    if (argc > 3) {
        same_fd_threads = atoi(argv[3]);
    }
    if (argc > 4) {
        same_fd_iterations = atoi(argv[4]);
    }
    if (argc > 5) {
        multi_fd_threads = atoi(argv[5]);
    }
    if (argc > 6) {
        multi_fd_iterations = atoi(argv[6]);
    }
    if (same_fd_threads < 1) same_fd_threads = 8;
    if (same_fd_iterations < 1) same_fd_iterations = 2000;
    if (multi_fd_threads < 1) multi_fd_threads = 8;
    if (multi_fd_iterations < 1) multi_fd_iterations = 2000;

    printf("target=%s:%d\n", plc_ip, plc_port);
    printf("protocol=%s\n", use_ascii_protocol ? "ascii" : "bin");
    printf("same-fd: threads=%d, iterations=%d\n", same_fd_threads, same_fd_iterations);
    printf("multi-fd: threads=%d, iterations=%d\n", multi_fd_threads, multi_fd_iterations);

    configure_stress_protocol(use_ascii_protocol);

    int ascii_ret = 0;
    int same_fd_ret = 0;
    int multi_fd_ret = 0;

    // Use the same stress workflow for both bin and ascii.
    same_fd_ret = run_same_fd_stress(plc_ip, plc_port, same_fd_threads, same_fd_iterations);
    run_sleep_ms(300);
    multi_fd_ret = run_multi_fd_stress(plc_ip, plc_port, multi_fd_threads, multi_fd_iterations);

    mc_network_cleanup();

    if (ascii_ret != 0 || same_fd_ret != 0 || multi_fd_ret != 0) {
        return -1;
    }

    return 0;
}
