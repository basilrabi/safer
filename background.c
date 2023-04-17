#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>
#include <time.h>

int main(int argc, char **argv)
{
  char time_buffer[20];
  char *location = "LOCATION"; // Temporary placeholder for GNSS query
  char *message;
  int connected = 0;
  // Number of seconds before the new equipment status is recorded to allow making mistakes
  int seconds_refresh_cutoff = 10;
  // Number of seconds between location queries
  int seconds_location_period = 30;
  redisContext *context = NULL;
  redisReply *equipment_status = NULL;
  redisReply *previous_equipment_status = NULL;
  redisReply *refresh_status = NULL;
  redisReply *set_command = NULL;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;

  while (connected == 0) {
    context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
        redisFree(context);
      } else {
        sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
      }
    } else {
      connected = 1;
    }
  }

  // TODO: Connect via serial connection

  time(&refresh_time);

  while (connected == 1) {
    time(&current_time);
    equipment_status = redisCommand(context, "GET equipment_status");
    previous_equipment_status = redisCommand(context, "GET previous_equipment_status");
    refresh_status = redisCommand(context, "GET status_refresh");
    if (equipment_status == NULL ||
        previous_equipment_status == NULL ||
        refresh_status == NULL) {
      sd_journal_send("MESSAGE=%s", "Failed to get redis response.", "PRIORITY=%i", LOG_ERR, NULL);
      continue;
    }

    if (equipment_status->type == REDIS_REPLY_STRING) {
      if (refresh_status->type == REDIS_REPLY_STRING &&
          strcmp(refresh_status->str, "1") == 0) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING &&
            strcmp(previous_equipment_status->str, equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            set_command = (redisReply *) redisCommand(context, "SET status_refresh 0");
            freeReplyObject(set_command);
            // TODO: GNSS query
            time_print = localtime(&current_time);
            strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
            message = (char *) malloc((strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3) * sizeof(char));
            strcpy(message, time_buffer);
            strcat(message, " ");
            strcat(message, equipment_status->str);
            strcat(message, " ");
            strcat(message, location);
            set_command = (redisReply *) redisCommand(context, "RPUSH messages %s", message);
            free(message);
            freeReplyObject(set_command);
            refresh_time = current_time;
          }
        } else {
          set_command = (redisReply *) redisCommand(context, "SET previous_equipment_status %s", equipment_status->str);
          freeReplyObject(set_command);
          refresh_time = current_time;
        }
      } else {
        if ((refresh_time + seconds_location_period) <= current_time) {
          // TODO: GNSS query
          time_print = localtime(&current_time);
          strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H:%M:%S", time_print);
          message = (char *) malloc((strlen(time_buffer) + strlen(equipment_status->str) + strlen(location) + 3) * sizeof(char));
          strcpy(message, time_buffer);
          strcat(message, " ");
          strcat(message, equipment_status->str);
          strcat(message, " ");
          strcat(message, location);
          set_command = (redisReply *) redisCommand(context, "RPUSH messages %s", message);
          free(message);
          freeReplyObject(set_command);
          refresh_time = current_time;
        }
      }
    }

    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(refresh_status);
  }
  return 0;
}
