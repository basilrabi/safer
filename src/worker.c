#include <hiredis/hiredis.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include "worker.h"
#include "utils.h"

void shutdown_trigger()
{
  int proceed_shutdown = 0;
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
      if (!redis_cmd("SET proceed_shutdown 0"))
        continue;
      pid_t pid = getpid();
      kill(pid, SIGKILL); // TODO: when the hardware is set-up, this should shutdown the system.
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
  int pre_shutdown = 0;
  time_t current_time;
  time_t refresh_time;
  time(&refresh_time);
  while (1) {
    sleep(1);
    time(&current_time);
    get_int_key("pre_shutdown", &pre_shutdown);
    if (pre_shutdown) {
      if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
        if (!redis_cmd("SET shutdown 1"))
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
  const char *location = "LOCATION"; // Temporary placeholder for GNSS query
  const char *last_status = "off";
  int buffer_size;
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
    redisReply *equipment_status = redisCommand(context, "GET equipment_status");
    redisReply *previous_equipment_status = redisCommand(context, "GET previous_equipment_status");
    redisReply *status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (equipment_status == NULL || previous_equipment_status == NULL || status_queue == NULL)
      {
        sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
        if (equipment_status != NULL)
          freeReplyObject(equipment_status);
        if (previous_equipment_status != NULL)
          freeReplyObject(previous_equipment_status);
        if (status_queue != NULL)
          freeReplyObject(status_queue);
        continue;
      }
    get_int_key("status_refresh", &refresh_status);
    get_int_key("shutdown", &shutdown);
    if (shutdown) {
      redisReply *pre_shutdown_time = redisCommand(context, "GET pre_shutdown_time");
      if (pre_shutdown_time == NULL) {
        freeReplyObject(equipment_status);
        freeReplyObject(previous_equipment_status);
        freeReplyObject(status_queue);
        continue;
      }
      if (pre_shutdown_time->type != REDIS_REPLY_STRING) {
        freeReplyObject(equipment_status);
        freeReplyObject(pre_shutdown_time);
        freeReplyObject(previous_equipment_status);
        freeReplyObject(status_queue);
        continue;
      }
      buffer_size = strlen(pre_shutdown_time->str) + strlen(last_status) + strlen(location) + 3;
      message = (char *) malloc(buffer_size * sizeof(char));
      memset(message, 0, buffer_size * sizeof(char));
      strcpy(message, pre_shutdown_time->str);
      strcat(message, " ");
      strcat(message, last_status);
      strcat(message, " ");
      strcat(message, location);
      push_message_status = push_message(context, message);
      free(message);
      if (!push_message_status) {
        freeReplyObject(equipment_status);
        freeReplyObject(pre_shutdown_time);
        freeReplyObject(previous_equipment_status);
        freeReplyObject(status_queue);
        continue;
      }
    }

    // TODO: when the new operator is different from the previous, update
    // TODO: when the new supervisor is different from the previous, update
    // TODO: GNSS query

    // Send SMS when the message queue limit is reached or there is shutdown signal.
    if (status_queue->type == REDIS_REPLY_ARRAY && (status_queue->elements >= message_limit || shutdown)) {
      if (send_equipment_status(context) != 0)
        continue;
      if (!redis_cmd("SET proceed_shutdown 1"))
        continue;
    }

    if (equipment_status->type == REDIS_REPLY_STRING && !shutdown) {
      if (refresh_status) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING && strcmp(previous_equipment_status->str, equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            if (!redis_cmd("SET status_refresh 0")) {
              freeReplyObject(equipment_status);
              freeReplyObject(previous_equipment_status);
              freeReplyObject(status_queue);
              continue;
            }
            time_print = localtime(&refresh_time);
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
            buffer_size = strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3;
            message = (char *) malloc(buffer_size * sizeof(char));
            memset(message, 0, buffer_size * sizeof(char));
            strcpy(message, time_buffer);
            strcat(message, " ");
            strcat(message, equipment_status->str);
            strcat(message, " ");
            strcat(message, location);
            push_message_status = push_message(context, message);
            free(message);
            if (!push_message_status) {
              freeReplyObject(equipment_status);
              freeReplyObject(previous_equipment_status);
              freeReplyObject(status_queue);
              continue;
            }
            refresh_time = current_time;
          }
        } else {
          if (!redis_cmd("SET previous_equipment_status %s", equipment_status->str)) {
            freeReplyObject(equipment_status);
            freeReplyObject(previous_equipment_status);
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
          buffer_size = strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3;
          message = (char *) malloc(buffer_size * sizeof(char));
          memset(message, 0, buffer_size * sizeof(char));
          strcpy(message, time_buffer);
          strcat(message, " ");
          strcat(message, equipment_status->str);
          strcat(message, " ");
          strcat(message, location);
          push_message_status = push_message(context, message);
          free(message);
          if (!push_message_status) {
            freeReplyObject(equipment_status);
            freeReplyObject(previous_equipment_status);
            freeReplyObject(status_queue);
            continue;
          }
          refresh_time = current_time;
        }
      }
    }
    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(status_queue);

    if (shutdown)
      break;
  }
  redisFree(context);
  return;
}
