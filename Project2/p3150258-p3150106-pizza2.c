#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "p3150258-p3150106-pizza2.h"

void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td) {
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0) {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    } else if (td->tv_sec < 0 && td->tv_nsec > 0) {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}

struct timespec startClock() {
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    return start;
}

struct timespec stopClock(struct timespec start) {
    struct timespec finish, delta;
    clock_gettime(CLOCK_REALTIME, &finish);
    sub_timespec(start, finish, &delta);
    return delta;
}

unsigned getRandomNumber(unsigned * seed, unsigned min, unsigned max) {
    unsigned range = max - min + 1;
    return (rand_r(seed) % (range)) +min;
}

double average_delay_deliver = 0;
double max_delay_deliver = 0;
double average_delay_cold = 0;
double max_delay_cold = 0;
unsigned active_cooks = 0;
unsigned active_ovens = 0;
unsigned active_deliverers = 0;
pthread_mutex_t printlock, avglock, maxlock, cooklock, ovenlock, deliverlock;
pthread_cond_t cond_cook, cond_bake, cond_deliver;

void check(int x) {
    if (x == -1) {
        printf("pthread failed \n");
        exit(1);
    }
}

void * customer(void *arg) {
    Order * order = (Order*) arg;

    pthread_mutex_lock(&printlock);
    printf("Customer will arrive with ID: %d will arrive at %d for %d pizzas \n", order->id, order->arrivalTime, order->how_many_pizzas);
    pthread_mutex_unlock(&printlock);

    sleep(order->arrivalTime);

    // arrive, start clock
    struct timespec order_start = startClock();

    pthread_mutex_lock(&printlock);
    printf("Customer arrived with ID: %d for %d pizzas \n", order->id, order->how_many_pizzas);
    printf("waiting for a cook \n");
    pthread_mutex_unlock(&printlock);

    // --------------------------------------------------- wait for preparation
    pthread_mutex_lock(&cooklock);
    while (active_cooks == 0) {
        pthread_mutex_lock(&printlock);
        printf("waiting for a cook for %d \n", order->id);
        pthread_mutex_unlock(&printlock);

        pthread_cond_wait(&cond_cook, &cooklock);
    }
    active_cooks -= 1;
    pthread_mutex_unlock(&cooklock);

    // --------------------------------------------------- prepare
    pthread_mutex_lock(&printlock);
    printf("*** Preparing pizza for %d \n", order->id);
    pthread_mutex_unlock(&printlock);

    sleep(order->how_many_pizzas * T_PREP); // prepare

    // --------------------------------------------------- wait for oven
    pthread_mutex_lock(&ovenlock);
    while (active_ovens == 0) {
        pthread_mutex_lock(&printlock);
        printf("waiting for a oven for %d \n", order->id);
        pthread_mutex_unlock(&printlock);

        pthread_cond_wait(&cond_bake, &ovenlock);
    }
    active_ovens -= 1;
    pthread_mutex_unlock(&ovenlock);

    // --------------------------------------------------- release cook
    pthread_mutex_lock(&cooklock);
    active_cooks += 1;
    pthread_mutex_unlock(&cooklock);

    pthread_cond_broadcast(&cond_cook);

    // --------------------------------------------------- bake
    pthread_mutex_lock(&printlock);
    printf("*** Baking pizza for %d \n", order->id);
    pthread_mutex_unlock(&printlock);

    sleep(T_BAKE); // bake

    struct timespec cold_start = startClock(); // start clock for cold time

    // --------------------------------------------------- wait for deliverer
    pthread_mutex_lock(&deliverlock);
    while (active_deliverers == 0) {
        pthread_mutex_lock(&printlock);
        printf("waiting for a deliverer for %d \n", order->id);
        pthread_mutex_unlock(&printlock);

        pthread_cond_wait(&cond_deliver, &deliverlock);
    }
    active_deliverers -= 1;
    pthread_mutex_unlock(&deliverlock);

    // --------------------------------------------------- release oven
    pthread_mutex_lock(&ovenlock);
    active_ovens += 1;
    pthread_mutex_unlock(&ovenlock);

    pthread_cond_broadcast(&cond_bake);

    // --------------------------------------------------- drive to the customer

    sleep(order->deliverTime);

    // --------------------------------------------------- deliver

    struct timespec order_delta = stopClock(order_start);

    double seconds = ((int) order_delta.tv_sec) + order_delta.tv_nsec / 1000000000.0;

    pthread_mutex_lock(&maxlock);
    if (seconds > max_delay_deliver) {
        max_delay_deliver = seconds;
    }
    pthread_mutex_unlock(&maxlock);

    pthread_mutex_lock(&avglock);
    average_delay_deliver = average_delay_deliver + seconds;
    pthread_mutex_unlock(&avglock);



    struct timespec cold_delta = stopClock(cold_start);

    double cold_seconds = ((int) cold_delta.tv_sec) + cold_delta.tv_nsec / 1000000000.0;

    pthread_mutex_lock(&printlock);
    printf("Customer with ID: %d received his pizza, order delta: %d.%.9ld, s=%lf - order delta: %d.%.9ld, s=%lf\n", order->id, (int) order_delta.tv_sec, order_delta.tv_nsec, seconds, (int) cold_delta.tv_sec, cold_delta.tv_nsec, cold_seconds);
    pthread_mutex_unlock(&printlock);

    pthread_mutex_lock(&maxlock);
    if (cold_seconds > max_delay_cold) {
        max_delay_cold = cold_seconds;
    }
    pthread_mutex_unlock(&maxlock);

    pthread_mutex_lock(&avglock);
    average_delay_cold = average_delay_cold + cold_seconds;
    pthread_mutex_unlock(&avglock);

    // --------------------------------------------------- drive back

    sleep(order->deliverTime);

    // --------------------------------------------------- release deliverer

    pthread_mutex_lock(&deliverlock);
    active_deliverers += 1;
    pthread_mutex_unlock(&deliverlock);

    pthread_cond_broadcast(&cond_deliver);


    free(order);
    pthread_exit(0);
    return 0;
}

int main(int argc, char** argv) {
    unsigned ncust = 0;
    unsigned seed = 0;
    unsigned i = 0;

    switch (argc) {
        case 3:
            ncust = atoi(argv[1]);
            seed = atoi(argv[2]);
            break;
    }

    if (ncust == 0 || seed == 0) {
        printf("Invalid value or missing value for ncust or seed \n");
        return 0;
    } else {
        srand(seed);
        active_cooks = N_COOK;
        active_ovens = N_OVEN;
        active_deliverers = N_DELIVERERS;
    }

    unsigned arrival = T0;

    pthread_t * ids = malloc(sizeof (pthread_t) * ncust);

    check(pthread_mutex_init(&printlock, 0));
    check(pthread_mutex_init(&avglock, 0));
    check(pthread_mutex_init(&maxlock, 0));
    check(pthread_mutex_init(&cooklock, 0));
    check(pthread_mutex_init(&ovenlock, 0));
    check(pthread_mutex_init(&deliverlock, 0));
    check(pthread_cond_init(&cond_bake, NULL));
    check(pthread_cond_init(&cond_cook, NULL));
    check(pthread_cond_init(&cond_deliver, NULL));

    for (i = 0; i < ncust; i++) {
        pthread_t temp;

        pthread_mutex_lock(&printlock);
        printf("Creating customer: %d, arrival time at: %d sec \n", i, arrival);
        pthread_mutex_unlock(&printlock);

        Order * order = malloc(sizeof (Order));
        order->id = i;
        order->arrivalTime = arrival;
        order->how_many_pizzas = getRandomNumber(&seed, N_ORDER_LOW, N_ORDER_HIGH);
        order->completionTime = 0;
        order->deliverTime = getRandomNumber(&seed, T_LOW, T_HIGH);

        pthread_create(&temp, NULL, customer, order);

        ids[i] = temp;
        arrival = arrival + getRandomNumber(&seed, T_ORDER_LOW, T_ORDER_HIGH);
    }

    pthread_mutex_lock(&printlock);
    printf("waiting for all customers to be serviced \n");
    pthread_mutex_unlock(&printlock);


    for (i = 0; i < ncust; i++) {
        pthread_join(ids[i], NULL);
    }

    free(ids);

    printf("                 Order service stats \n");
    printf("----------------------------------------------- \n");
    printf("  Average delay: %.5lf \n", average_delay_deliver / ncust);
    printf("  Maximum delay: %.5lf \n", max_delay_deliver);
    printf("----------------------------------------------- \n");
    
    printf("\n");
    printf("                 Order cold stats \n");
    printf("----------------------------------------------- \n");
    printf("  Average delay: %.5lf \n", average_delay_cold / ncust);
    printf("  Maximum delay: %.5lf \n", max_delay_cold);
    printf("----------------------------------------------- \n");

    check(pthread_mutex_destroy(&printlock));
    check(pthread_mutex_destroy(&avglock));
    check(pthread_mutex_destroy(&maxlock));
    check(pthread_mutex_destroy(&cooklock));
    check(pthread_mutex_destroy(&ovenlock));
    check(pthread_mutex_destroy(&deliverlock));
    check(pthread_cond_destroy(&cond_bake));
    check(pthread_cond_destroy(&cond_cook));
    check(pthread_cond_destroy(&cond_deliver));
    
    return 0;
}

