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

#include "melsec_mc_bin.h"
#include "network_init.h"

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

    for (int i = 0; i < arg->iterations; i++) {
        int32 write_d = (int32)((arg->thread_id << 20) ^ i);
        int32 read_d = 0;
        bool write_m = ((i & 1) != 0);
        bool read_m = false;

        mc_error_code_e ret = mc_write_int32(arg->fd, arg->d_addr, write_d);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        ret = mc_read_int32(arg->fd, arg->d_addr, &read_d);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        arg->checks++;
        if (read_d != write_d) {
            arg->mismatches++;
        }

        ret = mc_write_bool(arg->fd, arg->m_addr, write_m);
        arg->api_calls++;
        if (ret != MC_ERROR_CODE_SUCCESS) {
            arg->io_errors++;
            continue;
        }

        ret = mc_read_bool(arg->fd, arg->m_addr, &read_m);
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
    int fd = mc_connect((char*)ip, port, 0, 0);
    if (fd <= 0) {
        printf("[same-fd] connect failed: %s:%d\n", ip, port);
        return -1;
    }

    stress_worker_arg_t* args = (stress_worker_arg_t*)calloc((size_t)thread_count, sizeof(stress_worker_arg_t));
    if (args == NULL) {
        mc_disconnect(fd);
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
    mc_disconnect(fd);

    return thread_ret;
}

static int run_multi_fd_stress(const char* ip, int port, int thread_count, int iterations)
{
    int* fds = (int*)calloc((size_t)thread_count, sizeof(int));
    stress_worker_arg_t* args = (stress_worker_arg_t*)calloc((size_t)thread_count, sizeof(stress_worker_arg_t));
    if (fds == NULL || args == NULL) {
        free(fds);
        free(args);
        return -1;
    }

    for (int i = 0; i < thread_count; i++) {
        fds[i] = mc_connect((char*)ip, port, 0, 0);
        if (fds[i] <= 0) {
            printf("[multi-fd] connect failed at idx=%d: %s:%d\n", i, ip, port);
            for (int j = 0; j < i; j++) {
                if (fds[j] > 0) {
                    mc_disconnect(fds[j]);
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
            mc_disconnect(fds[i]);
        }
    }

    free(fds);
    free(args);
    return thread_ret;
}

int main(int argc, char** argv)
{
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
    printf("same-fd: threads=%d, iterations=%d\n", same_fd_threads, same_fd_iterations);
    printf("multi-fd: threads=%d, iterations=%d\n", multi_fd_threads, multi_fd_iterations);

    int same_fd_ret = run_same_fd_stress(plc_ip, plc_port, same_fd_threads, same_fd_iterations);
    run_sleep_ms(300);
    int multi_fd_ret = run_multi_fd_stress(plc_ip, plc_port, multi_fd_threads, multi_fd_iterations);

    mc_network_cleanup();

    if (same_fd_ret != 0 || multi_fd_ret != 0) {
        return -1;
    }

    return 0;
}
