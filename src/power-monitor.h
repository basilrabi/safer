#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <glib-object.h>

G_BEGIN_DECLS

#define POWER_MONITOR_TYPE (power_monitor_get_type())

G_DECLARE_FINAL_TYPE (PowerMonitor, power_monitor, SAFER, POWERMONITOR, GObject)

void         power_monitor_emit_battery_level_changed(PowerMonitor *self);
void         power_monitor_emit_voltage_level_changed(PowerMonitor *self);
const gchar *power_monitor_get_battery               (PowerMonitor *self);
const gchar *power_monitor_get_voltage               (PowerMonitor *self);
void         power_monitor_set_battery               (PowerMonitor *self,
		                                      const gchar  *batteryLevel);
void         power_monitor_set_voltage               (PowerMonitor *self,
		                                      const gchar  *voltageLevel);

G_END_DECLS

#endif // POWER_MONITOR_H

