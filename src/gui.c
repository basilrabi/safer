#include "gui.h"
#include "gui_functions.h"
#include "utils.h"
#include "worker.h"

void activate(GtkApplication *app, gpointer data)
{
  pset *pointer_set = (pset *) data;
  GtkCssProvider *cssProvider;
  GtkWidget *boxActivity = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelOperator = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *boxPersonnelSupervisor = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
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
  GtkWidget *tabLabelActivity = gtk_label_new("Activity");
  GtkWidget *tabLabelPersonnel = gtk_label_new("Personnel");
  GtkWidget *window = gtk_application_window_new(app);
  guint boxPacking = 0;

  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxOperator), "operators", pointer_set->context);
  populate_comboboxtext(GTK_COMBO_BOX_TEXT(comboBoxSupervisor), "supervisors", pointer_set->context);

  gtk_container_add(GTK_CONTAINER(window), notebook);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), boxPersonnel, tabLabelPersonnel);

  // TODO: content for personnel
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

  g_signal_connect(buttonIdling, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonProduction, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonQueu, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonRefuelling, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonTravel, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(buttonWarmup, "clicked", G_CALLBACK(toggle_status), pointer_set->context);
  g_signal_connect(comboBoxOperator, "changed", G_CALLBACK(toggle_personnel), NULL);
  g_signal_connect(comboBoxSupervisor, "changed", G_CALLBACK(toggle_personnel), NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(buttonWarmup), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER(boxActivity), 5);
  gtk_widget_show_all (window);
  gtk_window_fullscreen (GTK_WINDOW(window));
  g_thread_new("PersonnelSenderThread", (GThreadFunc) personnel_sender, NULL);
  g_thread_new("StatusSenderThread", (GThreadFunc) status_sender, NULL);
  g_thread_new("ShutdownWatcherThread", (GThreadFunc) shutdown_watcher, NULL);
  g_thread_new("ShutdownTriggerThread", (GThreadFunc) shutdown_trigger, NULL);
}
