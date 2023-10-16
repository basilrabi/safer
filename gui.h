#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

/**
 * activate:
 * @app: a GtkApplication
 * @data: a pset to be used in activating @app
 *
 * Activates @app with all the UI components.
 */
void activate(GtkApplication *app,
              gpointer        data);

#endif // GUI_H
