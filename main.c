#include <gtk/gtk.h>

static void activate(GtkApplication* app, gpointer user_data)
{
  GtkWidget *buttonIdling;
  GtkWidget *buttonProduction;
  GtkWidget *buttonQueu;
  GtkWidget *buttonRefuelling;
  GtkWidget *buttonTravel;
  GtkWidget *buttonWarmup;
  GtkWidget *grid;
  GtkWidget *window;

  window = gtk_application_window_new (app);
  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (window), grid);

  buttonIdling     = gtk_button_new_with_label ("Idling"                  );
  buttonProduction = gtk_button_new_with_label ("Production"              );
  buttonQueu       = gtk_button_new_with_label ("Queuing"                 );
  buttonRefuelling = gtk_button_new_with_label ("Refuelling"              );
  buttonTravel     = gtk_button_new_with_label ("Travelling/Repositioning");
  buttonWarmup     = gtk_button_new_with_label ("Warm-up/Cooling"         );

  gtk_grid_attach (GTK_GRID (grid), buttonQueu      , 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), buttonTravel    , 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), buttonRefuelling, 0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), buttonIdling    , 0, 3, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), buttonWarmup    , 0, 4, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), buttonProduction, 0, 5, 1, 1);

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

