#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <periphery/gpio.h>

#define NUM_STEPS   4
#define NUM_LEDS    4
#define STEP_MS     500     /* 120 BPM quarter notes */
#define DEBOUNCE_MS 150

/* GPIO pin assignments */
static const int LED_PINS[NUM_LEDS] = {27, 23, 22, 24};
static const int BTN_PINS[NUM_LEDS] = {18, 17, 10, 25};

/* Sequencer pattern: pattern[step][led] */
static bool pattern[NUM_STEPS][NUM_LEDS];
static int  current_step = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static gpio_t *gpio_leds[NUM_LEDS];
static gpio_t *gpio_btns[NUM_LEDS];

static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Write current step's LED pattern to hardware */
static void apply_leds(int step) {
    for (int i = 0; i < NUM_LEDS; i++) {
        bool val;
        pthread_mutex_lock(&mutex);
        val = pattern[step][i];
        pthread_mutex_unlock(&mutex);
        if (gpio_write(gpio_leds[i], val) < 0)
            fprintf(stderr, "gpio_write LED%d: %s\n", i, gpio_errmsg(gpio_leds[i]));
    }
}

/* Sequencer clock — runs in the main thread */
static void run_sequencer(void) {
    while (true) {
        int step;
        pthread_mutex_lock(&mutex);
        step = current_step;
        pthread_mutex_unlock(&mutex);

        apply_leds(step);
        usleep(STEP_MS * 1000);

        pthread_mutex_lock(&mutex);
        current_step = (current_step + 1) % NUM_STEPS;
        pthread_mutex_unlock(&mutex);
    }
}

/* Wait until the button line has been quiet for debounce_ms.
   Any additional edge resets the settling timer, matching main.c logic. */
static void debounce_button(gpio_t *btn, unsigned int debounce_ms) {
    uint64_t press_time = get_time_ms();

    while (true) {
        int ret = gpio_poll(btn, debounce_ms);

        if (ret < 0) {
            fprintf(stderr, "gpio_poll(): %s\n", gpio_errmsg(btn));
            return;
        }

        gpio_edge_t edge;
        uint64_t event_time;

        if (ret > 0) {
            if (gpio_read_event(btn, &edge, &event_time) < 0) {
                fprintf(stderr, "gpio_read_event(): %s\n", gpio_errmsg(btn));
                return;
            }
            press_time = get_time_ms();
            continue;
        }

        uint64_t elapsed = get_time_ms() - press_time;
        if (elapsed >= debounce_ms)
            return;
    }
}

/* Button handler — one thread per button */
static void *button_thread(void *arg) {
    int idx = *(int *)arg;

    while (true) {
        int ret = gpio_poll(gpio_btns[idx], -1);
        if (ret < 0) {
            fprintf(stderr, "gpio_poll BTN%d: %s\n", idx, gpio_errmsg(gpio_btns[idx]));
            continue;
        }
        if (ret == 0)
            continue;

        gpio_edge_t edge;
        uint64_t event_time;
        if (gpio_read_event(gpio_btns[idx], &edge, &event_time) < 0) {
            fprintf(stderr, "gpio_read_event BTN%d: %s\n", idx, gpio_errmsg(gpio_btns[idx]));
            continue;
        }

        /* Wait for the line to settle before acting */
        debounce_button(gpio_btns[idx], DEBOUNCE_MS);

        /* Toggle LED idx for the current step and reflect immediately */
        pthread_mutex_lock(&mutex);
        int step = current_step;
        pattern[step][idx] = !pattern[step][idx];
        bool new_val = pattern[step][idx];
        pthread_mutex_unlock(&mutex);

        if (gpio_write(gpio_leds[idx], new_val) < 0)
            fprintf(stderr, "gpio_write LED%d: %s\n", idx, gpio_errmsg(gpio_leds[idx]));

        printf("Step %d: LED%d -> %s\n", step + 1, idx + 1, new_val ? "ON" : "OFF");
        fflush(stdout);
    }
    return NULL;
}

int main(void) {
    memset(pattern, 0, sizeof(pattern));

    // setup LEDs 
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_leds[i] = gpio_new();
        if (gpio_open(gpio_leds[i], "/dev/gpiochip0", LED_PINS[i], GPIO_DIR_OUT) < 0) {
            fprintf(stderr, "gpio_open LED%d (gpio%d): %s\n",
                    i + 1, LED_PINS[i], gpio_errmsg(gpio_leds[i]));
            exit(1);
        }
    }

    // setup buttons
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_btns[i] = gpio_new();
        if (gpio_open(gpio_btns[i], "/dev/gpiochip0", BTN_PINS[i], GPIO_DIR_IN) < 0) {
            fprintf(stderr, "gpio_open BTN%d (gpio%d): %s\n",
                    i + 1, BTN_PINS[i], gpio_errmsg(gpio_btns[i]));
            exit(1);
        }
        if (gpio_set_edge(gpio_btns[i], GPIO_EDGE_FALLING) < 0) {
            fprintf(stderr, "gpio_set_edge BTN%d: %s\n", i + 1, gpio_errmsg(gpio_btns[i]));
            exit(1);
        }
    }

    printf("Sequencer started — %d steps @ %d ms/step\n", NUM_STEPS, STEP_MS);
    printf("Press a button to toggle that LED for the current step.\n");
    fflush(stdout);

    /* Start one button thread per button */
    pthread_t btn_threads[NUM_LEDS];
    int btn_idx[NUM_LEDS];
    for (int i = 0; i < NUM_LEDS; i++) {
        btn_idx[i] = i;
        if (pthread_create(&btn_threads[i], NULL, button_thread, &btn_idx[i]) != 0) {
            fprintf(stderr, "pthread_create BTN%d failed\n", i + 1);
            exit(1);
        }
    }

    /* Run sequencer clock in main thread (never returns) */
    run_sequencer();

    return 0;
}
