#include <Arduino.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <AceButton.h>
#include <wasm3.h>
#include <m3_env.h>
#include "soc/rtc_wdt.h"
#include "FS.h"
#include "SPIFFS.h"

#include "./data.h"

using namespace ace_button;

#define NUM_LEDS 25
#define LED_DATA_PIN 27
#define BUTTON_PIN 39
#define DISPLAY_SIZE 5
#define WASM_STACK_SLOTS (512)
#define WASM_MEMORY_LIMIT (2*1024)
#define NATIVE_STACK_SIZE (4*1024)
#define MAX_APP_INSTANCES 26
#define MAX_TIMERS 2
#define MAX_KVS 50

#define FATAL(func, msg) {           \
  Serial.print("Fatal: " func ": "); \
  Serial.println(msg); return; }

#define m3ApiGetApp(NAME) AppInstance *NAME = (AppInstance*) m3_GetUserData(runtime);

struct Timer {
  bool active;
  int id;
  int interval;
  int previousRunTime;
};

struct ButtonEvent {
  bool active;
  int type;
  int button;
  int state;
};

struct AppInstance {
  bool active;
  int id;
  const char *path;
  Timer timers[MAX_TIMERS];
  bool subscribeToButtons;
  ButtonEvent buttonEvent;
};

struct WasmFile {
  const char *name;
  uint8_t *data;
  uint32_t size;
};

struct KeyValueEntry {
  bool active;
  char key[10];
  uint32_t value;
};

CRGB ledData[NUM_LEDS];
AceButton button(BUTTON_PIN);
KeyValueEntry kvs[MAX_KVS];
AppInstance apps[MAX_APP_INSTANCES];
IM3Environment env;
WasmFile files[] = {
  { .name = "startup.wasm", .data = wasm_startup_wasm, .size = wasm_startup_wasm_len },
  { .name = "blinker.wasm", .data = wasm_blinker_wasm, .size = wasm_blinker_wasm_len },
};

int runWasmFile(const char *path);

m3ApiRawFunction(log_v1_info) {
  m3ApiGetArgMem (const uint8_t *, buf)

  Serial.println((char*)buf);
  m3ApiSuccess();
}

m3ApiRawFunction(runtime_v1_run_file) {
  m3ApiReturnType (uint32_t)
  m3ApiGetArgMem (const uint8_t *, path)

  int instanceId = runWasmFile((const char*)path);
  m3ApiReturn(instanceId);
}

m3ApiRawFunction(random_v1_get_int_in_range) {
  m3ApiReturnType (uint32_t)
  m3ApiGetArg (uint32_t, min)
  m3ApiGetArg (uint32_t, max)

  m3ApiReturn(random(min, max));
}

m3ApiRawFunction(runtime_v1_get_instance_id) {
  m3ApiReturnType (uint32_t)
  m3ApiGetApp (app)

  m3ApiReturn(app->id);
}

m3ApiRawFunction(kv_v1_set_int) {
  m3ApiGetArgMem (const uint8_t *, key)
  m3ApiGetArg (uint32_t, value)

  for (int i = 0; i < MAX_KVS; i++) {
    if (kvs[i].active) {
      continue;
    }
    strcpy(kvs[i].key, (char*)key);
    kvs[i].value = value;
    kvs[i].active = true;
    break;
  }

  m3ApiSuccess();
}

m3ApiRawFunction(kv_v1_get_int) {
  m3ApiReturnType (uint32_t)
  m3ApiGetArgMem (const uint8_t *, key)
  m3ApiGetArg (uint32_t, defaultValue)

  for (int i = 0; i < MAX_KVS; i++) {
    if (!kvs[i].active) {
      continue;
    }
    if (strcmp(kvs[i].key, (char*)key) == 0) {
      m3ApiReturn(kvs[i].value);
    }
  }

  m3ApiReturn(defaultValue);
}

m3ApiRawFunction(display_v1_update) {
  m3ApiGetArg (uint32_t, x)
  m3ApiGetArg (uint32_t, y)
  m3ApiGetArg (uint32_t, width)
  m3ApiGetArg (uint32_t, height)
  m3ApiGetArgMem (const uint8_t *, buf)

  for (int cx = 0; cx < width; cx++) {
    for (int cy = 0; cy < height; cy++) {
      int offset = (cx + cy * width) * 3;
      int r = buf[offset + 0];
      int g = buf[offset + 1];
      int b = buf[offset + 2];
      ledData[(cx + x) + (cy + y) * DISPLAY_SIZE].setRGB(r, g, b);
    }
  }

  m3ApiSuccess();
}

m3ApiRawFunction(timer_v1_start) {
  m3ApiReturnType (uint32_t)
  m3ApiGetApp (app)
  m3ApiGetArg (uint32_t, interval)

  int index;
  for (index = 0; index < MAX_TIMERS; index++) {
    if (!app->timers[index].active) {
      break;
    }
  }

  Timer *timer = &app->timers[index];
  if (timer->active) {
    // No timers left
    m3ApiReturn(-1);
  }

  timer->id = index;
  timer->interval = interval;
  timer->previousRunTime = 0;
  timer->active = true;

  m3ApiReturn(0);
}

m3ApiRawFunction(timer_v1_stop) {
  m3ApiGetApp (app)
  m3ApiGetArg (uint32_t, timerId)

  if (timerId < 0 || timerId >= MAX_TIMERS) {
    m3ApiSuccess();
  }

  app->timers[timerId].active = false;
  m3ApiSuccess();
}

m3ApiRawFunction(buttons_v1_start) {
  m3ApiGetApp (app)

  app->subscribeToButtons = true;
  m3ApiSuccess();
}

m3ApiRawFunction(buttons_v1_stop) {
  m3ApiGetApp (app)

  app->subscribeToButtons = false;
  m3ApiSuccess();
}

M3Result LinkModules(IM3Runtime runtime) {
  IM3Module module = runtime->modules;

  m3_LinkRawFunction(module, "log_v1", "info", "v(*)", &log_v1_info);
  m3_LinkRawFunction(module, "runtime_v1", "run_file", "i(*)", &runtime_v1_run_file);
  m3_LinkRawFunction(module, "runtime_v1", "get_instance_id", "i()", &runtime_v1_get_instance_id);
  m3_LinkRawFunction(module, "display_v1", "update", "v(iiii*)", &display_v1_update);
  m3_LinkRawFunction(module, "timer_v1", "start", "i(i)", &timer_v1_start);
  m3_LinkRawFunction(module, "timer_v1", "stop", "v(i)", &timer_v1_stop);
  m3_LinkRawFunction(module, "kv_v1", "get_int", "i(*i)", &kv_v1_get_int);
  m3_LinkRawFunction(module, "kv_v1", "set_int", "v(*i)", &kv_v1_set_int);
  m3_LinkRawFunction(module, "random_v1", "get_int_in_range", "i(ii)", &random_v1_get_int_in_range);
  m3_LinkRawFunction(module, "buttons_v1", "start", "v()", &buttons_v1_start);
  m3_LinkRawFunction(module, "buttons_v1", "stop", "v()", &buttons_v1_stop);

  return m3Err_none;
}

void wasmThread(void *arg) {
  M3Result result = m3Err_none;
  AppInstance *app = (AppInstance*)arg;

  WasmFile *file = NULL;
  for (int i = 0; i < sizeof(files) / sizeof(WasmFile); i++) {
    if (strcmp(files[i].name, app->path) == 0) {
      file = &files[i];
      break;
    }
  }

  if (!file) FATAL("loadFile", "not found");

  IM3Runtime runtime = m3_NewRuntime(env, WASM_STACK_SLOTS, app);
  if (!runtime) FATAL("m3_NewRuntime", "failed");

  runtime->memoryLimit = WASM_MEMORY_LIMIT;

  IM3Module module;
  result = m3_ParseModule(env, &module, file->data, file->size);
  if (result) FATAL("m3_ParseModule", result);

  result = m3_LoadModule(runtime, module);
  if (result) FATAL("m3_LoadModule", result);

  result = LinkModules(runtime);
  if (result) FATAL("LinkModules", result);

  IM3Function f;
  result = m3_FindFunction(&f, runtime, "on_init");
  if (result) FATAL("m3_FindFunction init", result);
  result = m3_CallV(f);
  if (result) FATAL("m3_Call init", result);

  while (true) {
    int now = millis();

    for (int i = 0; i < MAX_TIMERS; i++) {
      Timer *timer = &app->timers[i];
      if (!timer->active) {
        continue;
      }

      if (now - timer->previousRunTime >= timer->interval) {
        result = m3_FindFunction(&f, runtime, "on_timer");
        if (result) FATAL("m3_FindFunction timer", result);
        result = m3_CallV(f, index);
        if (result) FATAL("m3_Call timer", result);

        timer->previousRunTime = now;
      }
    }

    if (app->buttonEvent.active) {
      app->buttonEvent.active = false;
      result = m3_FindFunction(&f, runtime, "on_button");
      if (result) FATAL("m3_FindFunction button", result);
      result = m3_CallV(f, app->buttonEvent.button, app->buttonEvent.type, app->buttonEvent.state);
      if (result) FATAL("m3_Call timer", result);
    }

    delay(1);
  }

  result = m3_FindFunction(&f, runtime, "on_destroy");
  if (result) FATAL("m3_FindFunction destroy", result);
  result = m3_CallV(f);
  if (result) FATAL("m3_Call destroy", result);

  Serial.println("Dune");
  app->active = false;
}

int runWasmFile(const char *path) {
  Serial.print("Starting ");
  Serial.print(path);
  Serial.print(", free heap ");
  Serial.println(ESP.getFreeHeap());
  Serial.println();

  for (int i = 0; i < MAX_APP_INSTANCES; i++) {
    if (apps[i].active) {
      continue;
    }

    apps[i].active = true;
    apps[i].id = i;
    apps[i].path = path;
    xTaskCreate(&wasmThread, "wasm3", NATIVE_STACK_SIZE, (void*)&apps[i], 5, NULL);
    return i;
  }

  Serial.println("No free apps");
  return -1;
}

void buttonHandler(AceButton *button, uint8_t eventType, uint8_t buttonState) {
  for (int i = 0; i < MAX_APP_INSTANCES; i++) {
    if (!apps[i].active || !apps[i].subscribeToButtons) {
      continue;
    }
    apps[i].buttonEvent.type = eventType;
    apps[i].buttonEvent.state = buttonState;
    apps[i].buttonEvent.button = 0; // ?
    apps[i].buttonEvent.active = true;
  }
}

void setup() {
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  button.setEventHandler(buttonHandler);

  FastLED.addLeds<WS2812, LED_DATA_PIN, GRB>(ledData, NUM_LEDS);
  FastLED.setBrightness(20);

  env = m3_NewEnvironment();
  runWasmFile("startup.wasm");
}

void loop() {
  button.check();
  FastLED.show();
}
