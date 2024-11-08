#include "gui.h"
#include "gui_functions.h"
#include "power-monitor.h"
#include "utils.h"
#include "worker.h"

void activate(GtkApplication *app, gpointer data)
{
  char *response = NULL;
  char *success = NULL;
  pset *pointer_set = (pset *) data;
  GtkCssProvider *cssProvider;
  GtkWidget *boxActivity = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxBrightness = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelOperator = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelSupervisor = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxSettings = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxLabelBrightness = gtk_label_new("Brightness");
  GtkWidget *boxLabelOperator = gtk_label_new("Operator");
  GtkWidget *boxLabelSupervisor = gtk_label_new("Supervisor");
  GtkWidget *buttonIdling = gtk_radio_button_new_with_label(NULL, "Idling");
  GtkWidget *buttonProduction;
  GtkWidget *buttonQueu;
  GtkWidget *buttonRefuelling;
  GtkWidget *buttonTravel;
  GtkWidget *buttonWarmup;
  GtkWidget *comboBoxOperator = gtk_combo_box_text_new();
  GtkWidget *comboBoxSupervisor = gtk_combo_box_text_new();
  GtkWidget *notebook = gtk_notebook_new();
  GtkWidget *labelBattery = gtk_label_new("Battery");
  GtkWidget *labelVoltage = gtk_label_new("Voltage");
  GtkWidget *levelBattery = gtk_label_new("---%");
  GtkWidget *levelVoltage = gtk_label_new("---V");
  GtkWidget *sliderBrightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
  GtkWidget *tabLabelActivity = gtk_label_new("Activity");
  GtkWidget *tabLabelPersonnel = gtk_label_new("Personnel");
  GtkWidget *tabLabelSettings = gtk_label_new("Settings");
  GtkWidget *tablePower = gtk_grid_new();
  GtkWidget *window = gtk_application_window_new(app);
  guint boxPacking = 0;
  PowerMonitor *powerStatus = g_object_new(POWER_MONITOR_TYPE, NULL);

  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxOperator), "operators", pointer_set->context);
  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxSupervisor), "supervisors", pointer_set->context);

  gtk_container_add(GTK_CONTAINER(window), notebook);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), boxPersonnel, tabLabelPersonnel);

  gtk_box_pack_start(GTK_BOX(boxPersonnelOperator), boxLabelOperator, FALSE, FALSE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxPersonnelOperator), comboBoxOperator, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxPersonnelSupervisor), boxLabelSupervisor, FALSE, FALSE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxPersonnelSupervisor), comboBoxSupervisor, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxPersonnel), boxPersonnelOperator, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxPersonnel), boxPersonnelSupervisor, TRUE, TRUE, boxPacking);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), boxActivity, tabLabelActivity);

  buttonProduction = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttonIdling), "Production");
  buttonQueu = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttonIdling), "Queuing");
  buttonRefuelling = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttonIdling), "Refueling");
  buttonTravel = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttonIdling), "Traveling/Repositioning");
  buttonWarmup = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttonIdling), "Warm-up/Cooling");

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), boxSettings, tabLabelSettings);

  gtk_box_pack_start(GTK_BOX(boxSettings), boxBrightness, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxBrightness), boxLabelBrightness, FALSE, FALSE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxBrightness), sliderBrightness, TRUE, TRUE, boxPacking);

  gtk_box_pack_start(GTK_BOX(boxSettings), tablePower, TRUE, TRUE, boxPacking + 5);
  gtk_grid_set_column_homogeneous(GTK_GRID(tablePower), TRUE);
  gtk_grid_set_row_homogeneous(GTK_GRID(tablePower), TRUE);
  gtk_grid_attach(GTK_GRID(tablePower), labelBattery, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(tablePower), labelVoltage, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(tablePower), levelBattery, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(tablePower), levelVoltage, 1, 1, 1, 1);

  cssProvider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(cssProvider, pointer_set->css, NULL);
  gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  gtk_widget_set_name(buttonIdling, "idle");
  gtk_widget_set_name(buttonProduction, "production");
  gtk_widget_set_name(buttonQueu, "queue");
  gtk_widget_set_name(buttonRefuelling, "refuel");
  gtk_widget_set_name(buttonTravel, "travel");
  gtk_widget_set_name(buttonWarmup, "warm-up");
  gtk_widget_set_name(comboBoxOperator, "operator");
  gtk_widget_set_name(comboBoxSupervisor, "supervisor");

  gtk_box_pack_start(GTK_BOX(boxActivity), buttonProduction, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxActivity), buttonQueu, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxActivity), buttonTravel, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxActivity), buttonRefuelling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxActivity), buttonIdling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start(GTK_BOX(boxActivity), buttonWarmup, TRUE, TRUE, boxPacking);

  gtk_range_set_value(GTK_RANGE(sliderBrightness), 50);

  g_signal_connect(buttonIdling, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonProduction, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonQueu, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonRefuelling, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonTravel, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonWarmup, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(comboBoxOperator, "changed", G_CALLBACK(toggle_personnel), NULL);
  g_signal_connect(comboBoxSupervisor, "changed", G_CALLBACK(toggle_personnel), NULL);
  g_signal_connect(powerStatus, "battery_level_changed", G_CALLBACK(update_battery), levelBattery);
  g_signal_connect(powerStatus, "voltage_level_changed", G_CALLBACK(update_voltage), levelVoltage);
  g_signal_connect(sliderBrightness, "value-changed", G_CALLBACK(adjust_brightness), NULL);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttonWarmup), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(boxActivity), 5);
  gtk_widget_show_all(window);
  gtk_window_fullscreen(GTK_WINDOW(window));

  while (success == NULL) {
    at_cmd("AT+CMGF=1", &response, 1);
    success = strstr(response, "OK");
    if (success == NULL)
      printf("Trying to set text mode for SMS again.\n");
  }
  success = NULL;
  while (success == NULL) {
    at_cmd("AT+CSCA=\"+639180000101\"", &response, 1);
    success = strstr(response, "OK");
    if (success == NULL)
      printf("Trying to set service center number again.\n");
  }

  g_thread_new("HatThread", (GThreadFunc) hat, NULL);
  g_thread_new("PersonnelSenderThread", (GThreadFunc) personnel_sender, NULL);
  g_thread_new("PowerMonitorThread", (GThreadFunc) power_monitor, powerStatus);
  g_thread_new("StatusSenderThread", (GThreadFunc) status_sender, NULL);
  g_thread_new("ShutdownWatcherThread", (GThreadFunc) shutdown_watcher, NULL);
  g_thread_new("ShutdownTriggerThread", (GThreadFunc) shutdown_trigger, NULL);
}

