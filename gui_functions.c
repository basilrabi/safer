#include <systemd/sd-journal.h>
#include "gui_functions.h"
#include "utils.h"

void populate_comboboxtext(GtkComboBoxText *box,
                           const char      *list,
                           redisContext    *context)
{
  redisReply *personnel_list = redisCommand(context, "LRANGE %s 0 -1", list);
  if (personnel_list == NULL) {
    sd_journal_send("MESSAGE=%s %s.", "Failed to get list of", list, "PRIORITY=%i", LOG_ERR, NULL);
  } else {
    if (personnel_list->type == REDIS_REPLY_ARRAY) {
      for (int counter = 0; counter < personnel_list->elements; counter++) {
        gtk_combo_box_text_append_text(box, personnel_list->element[counter]->str);
      }
    }
    freeReplyObject(personnel_list);
  }
}

void toggle_personnel(GtkWidget *box,
                      gpointer   data)
{
  redisContext *context = (redisContext *) data;
  int shutdown = 0;
  get_int_key(context, "shutdown", &shutdown);
  if (shutdown)
    return;
  if (gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box)) == NULL)
    redis_cmd("SET %s NONE", gtk_widget_get_name(box));
  else
    redis_cmd("SET %s %s", gtk_widget_get_name(box), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(box)));
  return;
}

void toggle_status(GtkWidget *button,
                   gpointer   data)
{
  redisContext *context = (redisContext *) data;
  int shutdown = 0;
  get_int_key(context, "shutdown", &shutdown);
  if (shutdown)
    return;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button))) {
    redisReply *previous;
    previous = redisCommand(context, "GET equipment_status");
    if (previous == NULL) {
      sd_journal_send("MESSAGE=%s", "Failed to get redis response after pressing button.", "PRIORITY=%i", LOG_ERR, NULL);
      return;
    }
    if (previous->type == REDIS_REPLY_STRING) {
      redis_cmd("SET previous_equipment_status %s", previous->str);
      if (strcmp(previous->str, gtk_widget_get_name(button)) != 0) {
        redis_cmd("SET equipment_status %s", gtk_widget_get_name(button));
        redis_cmd("SET status_refresh 1");
      }
    } else {
      redis_cmd("SET equipment_status %s", gtk_widget_get_name(button));
      redis_cmd("SET status_refresh 1");
    }
    freeReplyObject(previous);
  }
}

