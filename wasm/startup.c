#include <stdint.h>

#include "api.h"
#include "util.h"

int x = 0;
int y = 0;

void on_init() {
  buttons_start();
}

void add_new_pixel() {
  char key[10];

  int app_id = runtime_run_file("blinker.wasm");
  make_instanced_key(key, "x", app_id);
  kv_set_int(key, x);
  make_instanced_key(key, "y", app_id);
  kv_set_int(key, y);

  x += 1;
  if (x >= 5) {
    y += 1;
    x = 0;
  }
}

void on_button(int button, int event, int state) {
  if (event == 0) {
    add_new_pixel();
  }
}

void on_destroy() {
}

