#include <glib.h>
#include "power-monitor.h"

struct _PowerMonitor
{
  GObject parent_instance;
};

G_DEFINE_TYPE (PowerMonitor, power_monitor, G_TYPE_OBJECT)

enum {
  BATTERY_LEVEL_CHANGED,
  VOLTAGE_LEVEL_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void power_monitor_class_init(PowerMonitorClass *klass) {
  signals[BATTERY_LEVEL_CHANGED] =
    g_signal_new(
      "battery_level_changed",
      G_TYPE_FROM_CLASS(klass),
      G_SIGNAL_RUN_FIRST,
      0,
      NULL,
      NULL,
      NULL,
      G_TYPE_NONE,
      0
    );
  signals[VOLTAGE_LEVEL_CHANGED] =
    g_signal_new(
      "voltage_level_changed",
      G_TYPE_FROM_CLASS(klass),
      G_SIGNAL_RUN_FIRST,
      0,
      NULL,
      NULL,
      NULL,
      G_TYPE_NONE,
      0
    );
}

static void power_monitor_init(PowerMonitor *self) {}

void power_monitor_emit_battery_level_changed(PowerMonitor *self) {
  g_signal_emit(self, signals[BATTERY_LEVEL_CHANGED], 0);
}

void power_monitor_emit_voltage_level_changed(PowerMonitor *self) {
  g_signal_emit(self, signals[VOLTAGE_LEVEL_CHANGED], 0);
}

