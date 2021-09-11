#ifndef _SENSOR_API_H_
#define _SENSOR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((import_module("runtime_v1")))
__attribute__((import_name("run_file")))
extern int runtime_run_file(const char *);

__attribute__((import_module("runtime_v1")))
__attribute__((import_name("get_instance_id")))
extern int runtime_get_instance_id();

__attribute__((import_module("random_v1")))
__attribute__((import_name("get_int_in_range")))
extern int random_get_int_in_range(int min, int max);

__attribute__((import_module("log_v1")))
__attribute__((import_name("info")))
extern void log_info(const char *);

__attribute__((import_module("display_v1")))
__attribute__((import_name("update")))
extern void display_update(int x, int y, int width, int height, char *ptr);

__attribute__((import_module("timer_v1")))
__attribute__((import_name("start")))
extern int timer_start(int interval);

__attribute__((import_module("timer_v1")))
__attribute__((import_name("stop")))
extern void timer_stop(int timer_id);

__attribute__((import_module("buttons_v1")))
__attribute__((import_name("start")))
extern void buttons_start();

__attribute__((import_module("buttons_v1")))
__attribute__((import_name("stop")))
extern void buttons_stop();

__attribute__((import_module("kv_v1")))
__attribute__((import_name("set_int")))
extern void kv_set_int(const char *key, int value);

__attribute__((import_module("kv_v1")))
__attribute__((import_name("get_int")))
extern int kv_get_int(const char *key, int value);

#ifdef __cplusplus
}
#endif

#endif /* end of _SENSOR_API_H_ */

