#define _GNU_SOURCE

#include <hiredis/hiredis.h>
#include <stdio.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include "worker.h"
#include "utils.h"

void hat()
{
  char *response = NULL;
  int serial_file = -1;
  int shutdown = 0;
  while (serial_file < 0)
    get_int_key("serial_file", &serial_file);
  at_cmd("AT+GSMBUSY=1", &response, 1);
  while (shutdown == 0) {
    sleep(1);
    get_int_key("shutdown", &shutdown);
    // TODO: gnss query
    // TODO: queued SMS sending
    // TODO: read incmoming SMS (single part, multi-part)
  }
  return;
}

void personnel_sender()
{
  char *operator = NULL;
  char *previous_operator = NULL;
  char *previous_supervisor = NULL;
  char *supervisor = NULL;
  const int seconds_refresh_cutoff = 300;
  int refresh = 0;
  int shutdown = 0;
  time_t current_time;
  time_t refresh_time;
  time(&refresh_time);

  while (1) {
    sleep(1);
    time(&current_time);
    get_int_key("shutdown", &shutdown);
    if (shutdown)
      return;
    if (!get_char_key("operator", &operator) || !get_char_key("supervisor", &supervisor))
      continue;
    if (g_strcmp0(operator, "NONE") == 0 || g_strcmp0(supervisor, "NONE") == 0)
      continue;
    if (g_strcmp0(operator, previous_operator) != 0) {
      refresh = 1;
      str_copy(&previous_operator, operator);
      time(&refresh_time);
    }
    if (g_strcmp0(supervisor, previous_supervisor) != 0) {
      refresh = 1;
      str_copy(&previous_supervisor, supervisor);
      time(&refresh_time);
    }
    if (refresh) {
      if (g_strcmp0(operator, previous_operator) == 0 && g_strcmp0(supervisor, previous_supervisor) == 0) {
        if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
          send_sms("Operator: %s\nSupervisor:%s", operator, supervisor);
          refresh = 0;
          refresh_time = current_time;
        }
      }
    }
  }
}

void power_monitor(gpointer data)
{
  power_stat *powerStatus = (power_stat *) data;
}

void shutdown_trigger()
{
  int proceed_shutdown = 0;
  int serial_file = -1;
  while (1) {
    redisContext *context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
        redisFree(context);
      } else
        sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    } else
      break;
  }
  while (1) {
    sleep(1);
    get_int_key("proceed_shutdown", &proceed_shutdown);
    if (proceed_shutdown) {
      if (!redis_cmd("SET", "proceed_shutdown", "0"))
        continue;
      get_int_key("serial_file", &serial_file);
      if (serial_file < 0)
        continue;
      close(serial_file);
      system("sudo /usr/local/bin/x708-softsd.sh");
    }
  }
}

void shutdown_watcher()
{
  redisContext *context;
  while (1) {
    context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
        redisFree(context);
      } else
        sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    } else
      break;
  }
  // Number of seconds before the shutdown key is triggered.
  const int seconds_refresh_cutoff = 5;
  char time_buffer[20];
  int pre_shutdown = 0;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;
  time(&refresh_time);
  while (1) {
    sleep(1);
    time(&current_time);
    get_int_key("pre_shutdown", &pre_shutdown);
    if (pre_shutdown) {
      if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
        time_print = localtime(&refresh_time);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
        if (!redis_cmd("SET", "shutdown", "1") || !redis_cmd("SET", "shutdown_time", time_buffer))
          continue;
        else {
          redisFree(context);
          return;
        }
      }
    } else
      refresh_time = current_time;
  }

}

void status_sender()
{
  redisContext *context;
  while (1) {
    context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
        redisFree(context);
      } else
        sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    } else
      break;
  }

  // Number of messages to be queued before sending
  const int message_limit = 10;
  // Number of seconds before the new equipment status is recorded to allow making mistakes
  const int seconds_refresh_cutoff = 5;
  // Number of seconds between location queries
  const int seconds_location_period = 30;

  char time_buffer[20];
  char *message;
  char *equipment_status = NULL;
  char *pre_shutdown_time = NULL;
  char *previous_equipment_status = NULL;
  const char *location = "LOCATION"; // Temporary placeholder for GNSS query
  const char *last_status = "off";
  int push_message_status = 0;
  int refresh_status = 0;
  int shutdown = 0;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;
  time(&refresh_time);

  while (1) {
    sleep(1);
    time(&current_time);
    redisReply *status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (!get_char_key("equipment_status", &equipment_status)
        || !get_char_key("previous_equipment_status", &previous_equipment_status)
        || status_queue == NULL)
      {
        sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
        if (status_queue != NULL)
          freeReplyObject(status_queue);
        continue;
      }
    get_int_key("status_refresh", &refresh_status);
    get_int_key("shutdown", &shutdown);
    if (shutdown) {
      if (!get_char_key("pre_shutdown_time", &pre_shutdown_time)) {
        freeReplyObject(status_queue);
        continue;
      }
      if (asprintf(&message, "%s %s %s", pre_shutdown_time, last_status, location) < 0) {
        freeReplyObject(status_queue);
        continue;
      }
      push_message_status = redis_cmd("RPUSH", "messages", message);
      free(message);
      if (!push_message_status) {
        freeReplyObject(status_queue);
        continue;
      }
    }

    // TODO: GNSS query

    // Send SMS when the message queue limit is reached or there is shutdown signal.
    if (status_queue->type == REDIS_REPLY_ARRAY && (status_queue->elements >= message_limit || shutdown)) {
      if (send_equipment_status(context) != 0)
        continue;
      if (shutdown) {
        if (!redis_cmd("SET", "proceed_shutdown", "1"))
          continue;
      }
    }

    if (!shutdown) {
      if (refresh_status) {
        if (g_strcmp0(previous_equipment_status, equipment_status) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            if (!redis_cmd("SET", "status_refresh", "0")) {
              freeReplyObject(status_queue);
              continue;
            }
            time_print = localtime(&refresh_time);
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
            if (asprintf(&message, "%s %s %s", time_buffer, equipment_status, location) < 0) {
              freeReplyObject(status_queue);
              continue;
            }
            push_message_status = redis_cmd("RPUSH", "messages", message);
            free(message);
            if (!push_message_status) {
              freeReplyObject(status_queue);
              continue;
            }
            refresh_time = current_time;
          }
        } else {
          if (!redis_cmd("SET", "previous_equipment_status", equipment_status)) {
            freeReplyObject(status_queue);
            continue;
          }
          refresh_time = current_time;
        }
      } else {
        if ((refresh_time + seconds_location_period) <= current_time) {
          // TODO: GNSS query
          time_print = localtime(&current_time);
          strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
          if (asprintf(&message, "%s %s %s", time_buffer, equipment_status, location) < 0) {
            freeReplyObject(status_queue);
            continue;
          }
          push_message_status = redis_cmd("RPUSH", "messages", message);
          free(message);
          if (!push_message_status) {
            freeReplyObject(status_queue);
            continue;
          }
          refresh_time = current_time;
        }
      }
    }
    freeReplyObject(status_queue);

    if (shutdown)
      break;
  }
  g_free(equipment_status);
  g_free(pre_shutdown_time);
  g_free(previous_equipment_status);
  redisFree(context);
  return;
}
