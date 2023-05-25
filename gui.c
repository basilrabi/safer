#include <gtk/gtk.h>
#include <hiredis/hiredis.h>
#include <systemd/sd-journal.h>

static void populate_comboboxtext(GtkComboBoxText *box, const char *list) {
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s",
                      context->errstr,
                      "PRIORITY=%i",
                      LOG_ERR,
                      NULL);
      redisFree(context);
    } else {
      sd_journal_send("MESSAGE=%s",
                      "Connection error: can't allocate redis context",
                      "PRIORITY=%i",
                      LOG_ERR,
                      NULL);
    }
  }
  redisReply *personnel_list = redisCommand(context, "LRANGE %s 0 -1", list);
  if (personnel_list == NULL) {
    sd_journal_send(
      "MESSAGE=%s %s.",
      "Failed to get list of",
      list,
      "PRIORITY=%i",
      LOG_ERR,
      NULL
    );
  } else {
    if (personnel_list->type == REDIS_REPLY_ARRAY) {
      for (int counter = 0; counter < personnel_list->elements; counter++) {
        gtk_combo_box_text_append_text(box,
                                       personnel_list->element[counter]->str);
      }
    }
    freeReplyObject(personnel_list);
  }
  redisFree(context);
}

static void toggle_status(GtkWidget *button, gpointer data) {
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button))) {
    redisReply *previous;
    redisReply *reply;
    redisContext *context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s",
                        context->errstr,
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        redisFree(context);
      } else {
        sd_journal_send("MESSAGE=%s",
                        "Connection error: can't allocate redis context",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
      }
      return;
    }
    previous = redisCommand(context, "GET equipment_status");
    if (previous == NULL) {
      sd_journal_send("MESSAGE=%s",
                      "Failed to get redis response after pressing button.",
                      "PRIORITY=%i",
                      LOG_ERR,
                      NULL);
      return;
    }
    if (previous->type == REDIS_REPLY_STRING) {
      reply = redisCommand(context,
                           "SET previous_equipment_status %s",
                           previous->str);
      if (reply == NULL) {
        sd_journal_send("MESSAGE=%s",
                        "Failed to set previous_equipment_status.",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        return;
      } else {
        freeReplyObject(reply);
      }
      if (strcmp(previous->str, gtk_widget_get_name(button)) != 0) {
        reply = redisCommand(context,
                             "SET equipment_status %s",
                             gtk_widget_get_name(button));
        if (reply == NULL) {
          sd_journal_send("MESSAGE=%s",
                          "Failed to set previous_equipment_status.",
                          "PRIORITY=%i",
                          LOG_ERR,
                          NULL);
          return;
        } else {
          freeReplyObject(reply);
        }
        reply = redisCommand(context, "SET status_refresh 1");
        if (reply == NULL) {
          sd_journal_send("MESSAGE=%s",
                          "Failed to set status_refresh.",
                          "PRIORITY=%i",
                          LOG_ERR,
                          NULL);
          return;
        } else {
          freeReplyObject(reply);
        }
      }
    } else {
      reply = redisCommand(context,
                           "SET equipment_status %s",
                           gtk_widget_get_name(button));
      if (reply == NULL) {
        sd_journal_send("MESSAGE=%s",
                        "Failed to set fresh equipment_status.",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        return;
      } else {
        freeReplyObject(reply);
      }
      reply = redisCommand(context, "SET status_refresh 1");
      if (reply == NULL) {
        sd_journal_send("MESSAGE=%s",
                        "Failed to set fresh status_refresh.",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        return;
      } else {
        freeReplyObject(reply);
      }
    }
    freeReplyObject(previous);
    redisFree(context);
  }
}

static void activate(GtkApplication *app, gpointer user_data) {
  char *css = (char *) user_data;
  GtkCssProvider *cssProvider;
  GtkWidget *boxActivity = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelOperator = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelSupervisor = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxLabelOperator = gtk_label_new("Operator");
  GtkWidget *boxLabelSupervisor = gtk_label_new("Supervisor");
  GtkWidget *buttonIdling;
  GtkWidget *buttonProduction;
  GtkWidget *buttonQueu;
  GtkWidget *buttonRefuelling;
  GtkWidget *buttonTravel;
  GtkWidget *buttonWarmup;
  GtkWidget *comboBoxOperator = gtk_combo_box_text_new();
  GtkWidget *comboBoxSupervisor = gtk_combo_box_text_new();
  GtkWidget *notebook = gtk_notebook_new();
  GtkWidget *tabLabelActivity = gtk_label_new("Activity");
  GtkWidget *tabLabelPersonnel = gtk_label_new("Personnel");
  GtkWidget *window = gtk_application_window_new(app);
  guint boxPacking = 0;

  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxOperator), "operators");
  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxSupervisor), "supervisors");

  gtk_container_add(GTK_CONTAINER(window), notebook);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           boxPersonnel,
                           tabLabelPersonnel);

  // TODO: content for personnel
  gtk_box_pack_start(
    GTK_BOX(boxPersonnelOperator), boxLabelOperator, FALSE, FALSE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxPersonnelOperator), comboBoxOperator, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxPersonnelSupervisor), boxLabelSupervisor, FALSE, FALSE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxPersonnelSupervisor), comboBoxSupervisor, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxPersonnel), boxPersonnelOperator, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxPersonnel), boxPersonnelSupervisor, TRUE, TRUE, boxPacking
  );

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           boxActivity,
                           tabLabelActivity);

  buttonIdling = gtk_radio_button_new_with_label(NULL, "Idling");
  buttonProduction = gtk_radio_button_new_with_label_from_widget(
    GTK_RADIO_BUTTON(buttonIdling), "Production"
  );
  buttonQueu = gtk_radio_button_new_with_label_from_widget(
    GTK_RADIO_BUTTON(buttonIdling), "Queuing"
  );
  buttonRefuelling = gtk_radio_button_new_with_label_from_widget(
    GTK_RADIO_BUTTON(buttonIdling), "Refueling"
  );
  buttonTravel = gtk_radio_button_new_with_label_from_widget(
    GTK_RADIO_BUTTON(buttonIdling), "Traveling/Repositioning"
  );
  buttonWarmup = gtk_radio_button_new_with_label_from_widget(
    GTK_RADIO_BUTTON(buttonIdling), "Warm-up/Cooling"
  );

  cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, css, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                            GTK_STYLE_PROVIDER(cssProvider),
                                            GTK_STYLE_PROVIDER_PRIORITY_USER);

  gtk_widget_set_name(buttonIdling, "idle");
  gtk_widget_set_name(buttonProduction, "production");
  gtk_widget_set_name(buttonQueu, "queue");
  gtk_widget_set_name(buttonRefuelling, "refuel");
  gtk_widget_set_name(buttonTravel, "travel");
  gtk_widget_set_name(buttonWarmup, "warm-up");

  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonProduction, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonQueu, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonTravel, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonRefuelling, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonIdling, TRUE, TRUE, boxPacking
  );
  gtk_box_pack_start(
    GTK_BOX(boxActivity), buttonWarmup, TRUE, TRUE, boxPacking
  );

  g_signal_connect(buttonIdling,
                  "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);
  g_signal_connect(buttonProduction,
                   "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);
  g_signal_connect(buttonQueu,
                   "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);
  g_signal_connect(buttonRefuelling,
                   "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);
  g_signal_connect(buttonTravel,
                   "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);
  g_signal_connect(buttonWarmup,
                   "clicked",
                   G_CALLBACK(toggle_status),
                   NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(buttonWarmup), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER(boxActivity), 5);
  gtk_widget_show_all (window);
  gtk_window_fullscreen (GTK_WINDOW(window));
}

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

  app = gtk_application_new("com.nickelasia.tmc.datamanagement.safer",
                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), css);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

