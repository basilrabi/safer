#ifndef GUI_FUNCTIONS_H
#define GUI_FUNCTIONS_H

#include <gtk/gtk.h>
#include <hiredis/hiredis.h>
#include "power-monitor.h"

/**
 * adjust_brightness:
 * @slider: a GtkScale
 * @data: placeholder
 *
 * Adjust the brightness of the screen.
 * @box.
 **/
void adjust_brightness(GtkWidget *slider,
                       gpointer   data);


/**
 * populate_comboboxtext:
 * @box: a GtkComboBoxText
 * @list: the name of the list inside redis that will populate @box
 * @context: the redisContext to be used in connecting
 *
 * Populates the contents of @box.
 **/
void populate_comboboxtext(GtkComboBoxText *box,
                           const char      *list,
                           redisContext    *context);

/**
 * toggle_personnel:
 * @box: a GtkComboBoxText
 * @data: a redisContext to be used in connecting
 *
 * Sets the key in redis having the name of @box using the selected content of
 * @box.
 **/
void toggle_personnel(GtkWidget *box,
                      gpointer   data);

/**
 * toggle_status:
 * @button: a GtkToggleButton
 * @data: a redisContext to be used in connecting
 *
 * Sets the keys previous_equipment_status, equipment_status using the selected
 * @button.
 */
void toggle_status(GtkWidget *button,
                   gpointer   data);

/**
 * update_battery:
 * @data: the GtkLabel for battery
 *
 * Update the string of the battery label.
 */
void update_battery(PowerMonitor *monitor,
		    gpointer      data);

/**
 * update_voltage:
 * @data: the GtkLabel for voltage
 *
 * Update the string of the voltage label.
 */
void update_voltage(PowerMonitor *monitor,
		    gpointer      data);

#endif // GUI_FUNCTIONS_H

