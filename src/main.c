#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <systemd/sd-journal.h>
#include "gui.h"
#include "utils.h"

/**
 * main:
 *
 * Main function
 *
 * Returns: exit status of application
 *          -1 if failed to initialize redisContext
 *          -2 if failed to set redis keys
 *          -3 if failed to get css
 *          -4 if failed to open serial connection to the GNSS/GSM HAT
 */
int main(int argc, char **argv) {

  /* Initialize system time using RTC */
  system("sudo echo ds1307 0x68 > /sys/class/i2c-adapter/i2c-1/new_device");
  system("sudo hwclock -s");

  GtkApplication *app;
  char *css;
  char serial_str[20];
  const char *home_dir = (char *) g_get_home_dir();
  int hat = 0;
  int status;
  int serial_file;

  if (asprintf(&css, "%s/theme.css", home_dir) < 0)
    return -3;

  /* Activate HAT */
  while (!hat) {
    system("hat.py");
    sleep(1);
    get_int_key("hat", &hat);
  }

  initialize_serial_connection(&serial_file);
  if (serial_file < 0) {
    sd_journal_send("MESSAGE=Serial connection error.", "PRIORITY=%i", LOG_ERR, NULL);
    return -4;
  } else
    snprintf(serial_str, sizeof(serial_str), "%d", serial_file);
  app = gtk_application_new("com.nickelasia.tmc.datamanagement.safer", G_APPLICATION_DEFAULT_FLAGS);
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    free(css);
    return -1;
  }
  if (!redis_cmd("SET", "hat", "0") ||
      !redis_cmd("SET", "shutdown", "0") ||
      !redis_cmd("SET", "pre_shutdown", "0") ||
      !redis_cmd("SET", "proceed_shutdown", "0") ||
      !redis_cmd("SET", "operator", "NONE") ||
      !redis_cmd("SET", "serial_file", serial_str) ||
      !redis_cmd("SET", "supervisor", "NONE")) {
        close(serial_file);
        free(css);
        redisFree(context);
        return -2;
      }
  pset pointer_set;
  pointer_set.context = context;
  pointer_set.css = css;
  while (1) {
    if (set_hat_time())
      break;
  }
  g_signal_connect(app, "activate", G_CALLBACK(activate), &pointer_set);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  free(css);
  redisFree(context);
  return status;
}

