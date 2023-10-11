#include <gtk/gtk.h>
#include <hiredis/hiredis.h>
#include <systemd/sd-journal.h>
#include "gui.h"
#include "utils.h"

int main(int argc, char **argv) {
  GtkApplication *app;
  char *home_dir = (char *) g_get_home_dir();
  char *css;
  int buffer_size;
  int status;

  buffer_size = strlen(home_dir) + 11;
  css = (char *) malloc(buffer_size * sizeof(char));
  memset(css, 0, buffer_size * sizeof(char));
  strcpy(css, home_dir);
  strcat(css, "/theme.css");
  app = gtk_application_new("com.nickelasia.tmc.datamanagement.safer", G_APPLICATION_DEFAULT_FLAGS);
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else {
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    }
      return -1;
  }
  struct pset pointer_set;
  pointer_set.context = context;
  pointer_set.css = css;
  g_signal_connect(app, "activate", G_CALLBACK(activate), &pointer_set);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  free(css);
  redisFree(context);
  return status;
}

