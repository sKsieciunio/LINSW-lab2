#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <periphery/gpio.h>

#define DEBOUNCE_MS 150

static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void debounce_button(gpio_t *btn, unsigned int debounce_ms) {
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
    if (elapsed >= debounce_ms) {
      return;
    }
  }
}

int main(void) {
  gpio_t *gpio_led = gpio_new();
  gpio_t *gpio_btn = gpio_new();

  if (gpio_open(gpio_led, "/dev/gpiochip0", 27, GPIO_DIR_OUT) < 0) {
    fprintf(stderr, "gpio_open(): %s\n", gpio_errmsg(gpio_led));
    exit(1);
  }

  if (gpio_open(gpio_btn, "/dev/gpiochip0", 18, GPIO_DIR_IN) < 0) {
    fprintf(stderr, "gpio_open() button: %s\n", gpio_errmsg(gpio_btn));
    exit(1);
  }

  if (gpio_set_edge(gpio_btn, GPIO_EDGE_FALLING) < 0) {
    fprintf(stderr, "gpio_set_edge(): %s\n", gpio_errmsg(gpio_btn));
    exit(1);
  }

  bool led_state = false;

  while (true) {
    int ret = gpio_poll(gpio_btn, -1);
    if (ret < 0) {
      fprintf(stderr, "gpio_poll(): %s\n", gpio_errmsg(gpio_btn));
      exit(1);
    }

    gpio_edge_t event_edge;
    uint64_t event_time;
    if (gpio_read_event(gpio_btn, &event_edge, &event_time) < 0) {
      fprintf(stderr, "gpio_read_event(): %s\n", gpio_errmsg(gpio_btn));
      exit(1);
    }

    debounce_button(gpio_btn, DEBOUNCE_MS);

    led_state = !led_state;
    if (gpio_write(gpio_led, led_state) < 0) {
      fprintf(stderr, "gpio_write(): %s\n", gpio_errmsg(gpio_led));
      exit(1);
    }
  }

  gpio_close(gpio_led);
  gpio_free(gpio_led);
  gpio_close(gpio_btn);
  gpio_free(gpio_btn);
}