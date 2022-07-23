#include <gtk/gtk.h>

static void toggle_buttons(GtkWidget *button, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
  {
    g_print(gtk_widget_get_name(button));
    g_print("\n");
  }
}

static void activate(GtkApplication* app, gpointer user_data)
{
  GtkWidget *box;
  GtkWidget *buttonIdling;
  GtkWidget *buttonProduction;
  GtkWidget *buttonQueu;
  GtkWidget *buttonRefuelling;
  GtkWidget *buttonTravel;
  GtkWidget *buttonWarmup;
  GtkWidget *window;

  guint boxPacking = 0;

  window = gtk_application_window_new (app);
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add (GTK_CONTAINER (window), box);

  buttonIdling = gtk_radio_button_new_with_label (NULL, "Idling");
  buttonProduction = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonIdling), "Production");
  buttonQueu = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonIdling), "Queuing");
  buttonRefuelling = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonIdling), "Refuelling");
  buttonTravel = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonIdling), "Travelling/Repositioning");
  buttonWarmup = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (buttonIdling), "Warm-up/Cooling");

  gtk_widget_set_name(buttonIdling, "idling");
  gtk_widget_set_name(buttonProduction, "production");
  gtk_widget_set_name(buttonQueu, "queu");
  gtk_widget_set_name(buttonRefuelling, "refuelling");
  gtk_widget_set_name(buttonTravel, "travel");
  gtk_widget_set_name(buttonWarmup, "warmup");

  gtk_box_pack_start (GTK_BOX (box), buttonProduction, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonQueu, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonTravel, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonRefuelling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonIdling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonWarmup, TRUE, TRUE, boxPacking);

  g_signal_connect (buttonIdling, "clicked", G_CALLBACK (toggle_buttons), NULL);
  g_signal_connect (buttonProduction, "clicked", G_CALLBACK (toggle_buttons), NULL);
  g_signal_connect (buttonQueu, "clicked", G_CALLBACK (toggle_buttons), NULL);
  g_signal_connect (buttonRefuelling, "clicked", G_CALLBACK (toggle_buttons), NULL);
  g_signal_connect (buttonTravel, "clicked", G_CALLBACK (toggle_buttons), NULL);
  g_signal_connect (buttonWarmup, "clicked", G_CALLBACK (toggle_buttons), NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonWarmup), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER(box), 5);
  gtk_widget_show_all (window);
  gtk_window_fullscreen (GTK_WINDOW (window));
}

int main(int argc, char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new(NULL, G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

