#include <gtk/gtk.h>

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

  buttonIdling = gtk_toggle_button_new_with_label ("Idling");
  buttonProduction = gtk_toggle_button_new_with_label ("Production");
  buttonQueu = gtk_toggle_button_new_with_label ("Queuing");
  buttonRefuelling = gtk_toggle_button_new_with_label ("Refuelling");
  buttonTravel = gtk_toggle_button_new_with_label ("Travelling/Repositioning");
  buttonWarmup = gtk_toggle_button_new_with_label ("Warm-up/Cooling");

  gtk_box_pack_start (GTK_BOX (box), buttonQueu, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonTravel, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonRefuelling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonIdling, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonWarmup, TRUE, TRUE, boxPacking);
  gtk_box_pack_start (GTK_BOX (box), buttonProduction, TRUE, TRUE, boxPacking);

  gtk_container_set_border_width (GTK_CONTAINER(box), 5);
  gtk_widget_show_all (window);
  //gtk_window_fullscreen (GTK_WINDOW (window));
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

