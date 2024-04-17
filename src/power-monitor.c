#include <glib.h>
#include "power-monitor.h"

struct _PowerMonitor
{
  GObject parent_instance;
  gchar *battery;
  gchar *voltage;
};

G_DEFINE_TYPE (PowerMonitor, power_monitor, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_BATTERY,
  PROP_VOLTAGE,
  LAST_PROP
};

enum {
  BATTERY_LEVEL_CHANGED,
  VOLTAGE_LEVEL_CHANGED,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP];

static guint signals[LAST_SIGNAL];

static void power_monitor_class_get_property(GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec) {
  PowerMonitor *self = (PowerMonitor*) object;
  switch(prop_id) {
    case PROP_BATTERY:
      g_value_set_string(value, power_monitor_get_battery(self));
      break;
    case PROP_VOLTAGE:
      g_value_set_string(value, power_monitor_get_voltage(self));
      break;
  }
}

static void power_monitor_class_set_property(GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec) {
  PowerMonitor *self = (PowerMonitor*) object;
  switch(prop_id) {
    case PROP_BATTERY:
      power_monitor_set_battery(self, g_value_get_string(value));
      break;
    case PROP_VOLTAGE:
      power_monitor_set_voltage(self, g_value_get_string(value));
      break;
  }
}

static void power_monitor_class_init(PowerMonitorClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->get_property = power_monitor_class_get_property;
  object_class->set_property = power_monitor_class_set_property;

  properties[PROP_BATTERY] =
    g_param_spec_string(
      "battery",
      "Battery",
      "Battery level",
      "---%",
      (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
  properties[PROP_VOLTAGE] =
    g_param_spec_string(
      "voltage",
      "Voltage",
      "Voltage of power supply",
      "---V",
      (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );
  g_object_class_install_properties(object_class, LAST_PROP, properties);

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

const gchar* power_monitor_get_battery(PowerMonitor *self) {
  return self->battery;
}

const gchar* power_monitor_get_voltage(PowerMonitor *self) {
  return self->voltage;
}

void power_monitor_set_battery(PowerMonitor *self,
                               const gchar  *batteryLevel) {
  if (g_strcmp0(batteryLevel, self->battery) != 0) {
    g_free(self->battery);
    self->battery = g_strdup(batteryLevel);
  }
}

void power_monitor_set_voltage(PowerMonitor *self,
                               const gchar  *voltageLevel) {
  if (g_strcmp0(voltageLevel, self->voltage) != 0) {
    g_free(self->voltage);
    self->voltage = g_strdup(voltageLevel);
  }
}
