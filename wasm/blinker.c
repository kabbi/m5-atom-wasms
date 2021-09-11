#include <stdint.h>

#include "api.h"
#include "util.h"

char on_colors[] = {50, 0, 0};
char off_colors[] = {0, 0, 0};
int instance_id;
int on;

void on_init() {
  instance_id = runtime_get_instance_id();
  timer_start(random_get_int_in_range(100, 1000));
}

void on_timer() {
  on = !on;

  char key[10];
  make_instanced_key(key, "x", instance_id);
  int x = kv_get_int(key, 0);
  make_instanced_key(key, "y", instance_id);
  int y = kv_get_int(key, 0);
  display_update(x, y, 1, 1, on ? on_colors : off_colors);
}

void on_destroy() {
}

