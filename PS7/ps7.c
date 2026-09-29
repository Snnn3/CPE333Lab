#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_SEATS 100000L
#define OPERATIONS_PER_THREAD 100000L

static volatile long available_seats = INITIAL_SEATS;

typedef struct {
    long change;
    int force_context_switch;
} worker_args_t;

static void *update_seats(void *arg)
{
    const worker_args_t *worker = (const worker_args_t *)arg;

    for (long i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        long local_seats = available_seats;
        local_seats += worker->change;

        if (worker->force_context_switch) {
            sched_yield();
        }

        available_seats = local_seats;
    }

    return NULL;
}

static void *single_thread_baseline(void *unused)
{
    (void)unused;

    for (long i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        available_seats = available_seats - 1;
    }
    for (long i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        available_seats = available_seats + 1;
    }

    return NULL;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
            "Usage: %s [task]\n"
            "  (no arg) run all tasks (1, 2, and 3)\n"
            "  1        one-thread baseline\n"
            "  2        two threads, no deliberate yield\n"
            "  3        two threads, sched_yield between calculate and store\n",
            program);
}

static int run_single_thread_task(void)
{
    available_seats = INITIAL_SEATS;

    pthread_t thread;
    int error = pthread_create(&thread, NULL, single_thread_baseline, NULL);
    if (error != 0) {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(error));
        return EXIT_FAILURE;
    }

    error = pthread_join(thread, NULL);
    if (error != 0) {
        fprintf(stderr, "pthread_join failed: %s\n", strerror(error));
        return EXIT_FAILURE;
    }

    printf("Task 1: one-thread baseline\n");
    printf("Initial available seats: %ld\n", INITIAL_SEATS);
    printf("Expected final value:    %ld\n", INITIAL_SEATS);
    printf("Final available seats:   %ld\n", available_seats);
    return EXIT_SUCCESS;
}

static int run_race_task(int force_context_switch, int task_number)
{
    available_seats = INITIAL_SEATS;

    pthread_t threads[2];
    worker_args_t workers[2] = {
        {-1, force_context_switch},
        {+1, force_context_switch},
    };

    for (size_t i = 0; i < 2; ++i) {
        int error = pthread_create(&threads[i], NULL, update_seats, &workers[i]);
        if (error != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    for (size_t i = 0; i < 2; ++i) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(error));
            return EXIT_FAILURE;
        }
    }

    printf("Task %d: two threads%s\n", task_number,
           force_context_switch ? " with sched_yield" : " without deliberate yield");
    printf("Initial available seats: %ld\n", INITIAL_SEATS);
    printf("Expected final value:    %ld\n", INITIAL_SEATS);
    printf("Final available seats:   %ld\n", available_seats);
    return EXIT_SUCCESS;
}

static int run_all_tasks(void)
{
    int ret = run_single_thread_task();
    if (ret != EXIT_SUCCESS) {
        return ret;
    }
    printf("\n");

    ret = run_race_task(0, 2);
    if (ret != EXIT_SUCCESS) {
        return ret;
    }
    printf("\n");

    ret = run_race_task(1, 3);
    return ret;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        return run_all_tasks();
    }

    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *end = NULL;
    errno = 0;
    long task = strtol(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || task < 1 || task > 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    switch (task) {
    case 1:
        return run_single_thread_task();
    case 2:
        return run_race_task(0, 2);
    case 3:
        return run_race_task(1, 3);
    default:
        return EXIT_FAILURE;
    }
}
