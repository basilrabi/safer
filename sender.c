/* Daemon for sending messages to the server
 */

#define PCRE2_CODE_UNIT_WIDTH 8

#include <hiredis/hiredis.h>
#include <pcre2.h>
#include <stdlib.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include <unistd.h>

static void capture_pattern(const char *source,
                            char *datetime,
                            char *status,
                            char *location) {
  const PCRE2_SPTR pattern =
    (PCRE2_SPTR)
    "^(\\d{4}-\\d{2}-\\d{2}-\\d{2}:\\d{2}:\\d{2})\\s+([a-z\\-]+)\\s+(LOCATION).*";
  int error_code;
  int matches;
  pcre2_code *re;
  PCRE2_SIZE error_offset;
  re = pcre2_compile(pattern,
                     PCRE2_ZERO_TERMINATED,
                     0,
                     &error_code,
                     &error_offset,
                     NULL);
  if (re == NULL) {
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(error_code, buffer, sizeof(buffer));
    sd_journal_send("MESSAGE=%s",
                    "PCRE2 compilation failed at offset %d: %s",
                    (int) error_offset,
                    buffer,
                    "PRIORITY=%i",
                    LOG_ERR, NULL);
    return;
  }
  pcre2_match_data *match_data;
  match_data = pcre2_match_data_create_from_pattern(re, NULL);
  matches = pcre2_match(re,
                        (PCRE2_SPTR) source,
                        strlen(source),
                        0,
                        0,
                        match_data,
                        NULL);
  if (matches < 0) {
    switch(matches) {
      case PCRE2_ERROR_NOMATCH:
        sd_journal_send("MESSAGE=%s",
                        "PCRE2 no matches",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        break;
      default:
        sd_journal_send("MESSAGE=%s",
                        "PCRE2 matching error %d",
                        matches,
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        break;
    }
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return;
  }

  PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
  PCRE2_SPTR substring_datetime = (PCRE2_SPTR) source + ovector[2];
  strncpy(datetime, (char *) substring_datetime, ovector[3] - ovector[2]);
  datetime[ovector[3] - ovector[2]] = '\0';
  PCRE2_SPTR substring_status = (PCRE2_SPTR) source + ovector[4];
  strncpy(status, (char *) substring_status, ovector[5] - ovector[4]);
  status[ovector[5] - ovector[4]] = '\0';
  PCRE2_SPTR substring_location = (PCRE2_SPTR) source + ovector[6];
  strncpy(location, (char *) substring_location, ovector[7] - ovector[6]);
  location[ovector[7] - ovector[6]] = '\0';

  pcre2_code_free(re);
  pcre2_match_data_free(match_data);
  return;
}

// Saves the subset of the new string to `holder` starting from the index
// wherein the new string is different from the old string.
static void str_difference(const char *old, const char *new, char *holder) {
  int counter = 0;
  int idx;
  holder[0] = '\0';
  while (1) {
    if (old[counter] != new[counter]) {
      break;
    }
    counter += 1;
  }
  for (idx = 0; counter < strlen(new); idx++) {
    holder[idx] = new[counter];
    counter += 1;
  }
  holder[idx] = '\0';
}

static int prepare_message(redisContext *context) {
  int output = 0;
  redisReply *delete_messages = NULL;
  redisReply *status_queue = NULL;
  status_queue = redisCommand(context, "LRANGE messages 0 -1");
  if (status_queue == NULL) {
    sd_journal_send(
      "MESSAGE=%s",
      "Failed to get status queue while attempting to send message.",
      "PRIORITY=%i",
      LOG_ERR,
      NULL
    );
    output = 1;
  } else {
    if (status_queue->type == REDIS_REPLY_ARRAY) {
      char captured_datetime[20];
      char captured_equipment_status[11];
      char captured_location[10];
      char datetime_substr[20];
      char *messages;
      char new_captured_datetime[20];
      char new_captured_equipment_status[11];
      char new_captured_location[10];
      int buffer_size;
      int counter;
      int total_characters = 0;
      for (counter = 0; counter < status_queue->elements; counter++) {
        total_characters += strlen(status_queue->element[counter]->str);
      }
      buffer_size = total_characters + (status_queue->elements * 2);
      messages = (char *) malloc(buffer_size * sizeof(char));
      memset(messages, 0, buffer_size * sizeof(char));
      strcpy(messages, status_queue->element[0]->str);
      strcat(messages, "\n");
      capture_pattern(status_queue->element[0]->str,
                      captured_datetime,
                      captured_equipment_status,
                      captured_location);
      for (counter = 1; counter < status_queue->elements; counter++) {
        capture_pattern(status_queue->element[counter]->str,
                        new_captured_datetime,
                        new_captured_equipment_status,
                        new_captured_location);
        str_difference(captured_datetime,
                       new_captured_datetime,
                       datetime_substr);
        strcat(messages, datetime_substr);
        strcat(messages, " ");
        if (strcmp(captured_equipment_status,
                   new_captured_equipment_status) != 0) {
          strcat(messages, new_captured_equipment_status);
          strcat(messages, " ");
        }
        strcat(messages, new_captured_location);
        strcat(messages, "\n");
        strcpy(captured_datetime, new_captured_datetime);
        strcpy(captured_equipment_status, new_captured_equipment_status);
      }
      printf("All messages:\n");
      printf("%s\n", messages);
      free(messages);
      delete_messages = redisCommand(context, "DEL messages");
      if (delete_messages == NULL) {
        sd_journal_send("MESSAGE=%s",
                        "Failed to delete messages.",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        output = 3;
      } else {
        freeReplyObject(delete_messages);
      }
    } else {
      output = 2;
    }
    freeReplyObject(status_queue);
  }
  return output;
}

int main(int argc, char **argv) {
  // Number of messages to be queued before sending
  const int message_limit = 10;
  // Number of seconds before the new equipment status is recorded to allow making mistakes
  const int seconds_refresh_cutoff = 10;
  // Number of seconds between location queries
  const int seconds_location_period = 30;

  char time_buffer[20];
  char *location = "LOCATION"; // Temporary placeholder for GNSS query
  char *message;
  int buffer_size;
  int connected = 0;
  redisContext *context = NULL;
  redisReply *equipment_status = NULL;
  redisReply *previous_equipment_status = NULL;
  redisReply *refresh_status = NULL;
  redisReply *set_command = NULL;
  redisReply *status_queue = NULL;
  struct tm *time_print;
  time_t current_time;
  time_t refresh_time;

  // Connection Loop
  while (connected == 0) {
    context = redisConnect("localhost", 6379);
    if (context == NULL || context->err) {
      if (context) {
        sd_journal_send("MESSAGE=Connection error: %s",
                        context->errstr,
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
        redisFree(context);
      } else {
        sd_journal_send("MESSAGE=%s",
                        "Connection error: can't allocate redis context",
                        "PRIORITY=%i",
                        LOG_ERR,
                        NULL);
      }
    } else {
      connected = 1;
    }
  }

  // TODO: Connect via serial connection
  // TODO: Real time clock sync

  time(&refresh_time);

  // Main loop
  while (connected == 1) {
    sleep(1);
    time(&current_time);
    equipment_status = redisCommand(context, "GET equipment_status");
    previous_equipment_status = redisCommand(context,
                                             "GET previous_equipment_status");
    refresh_status = redisCommand(context, "GET status_refresh");
    status_queue = redisCommand(context, "LRANGE messages 0 -1");
    if (equipment_status == NULL ||
        previous_equipment_status == NULL ||
        refresh_status == NULL ||
        status_queue == NULL) {
      sd_journal_send("MESSAGE=%s",
                      "Failed to get redis response.",
                      "PRIORITY=%i",
                      LOG_ERR,
                      NULL);
      continue;
    }

    // Send SMS when the message queue limit is reached.
    if (status_queue->type == REDIS_REPLY_ARRAY &&
        status_queue->elements >= message_limit) {
      prepare_message(context);
    }

    if (equipment_status->type == REDIS_REPLY_STRING) {
      if (refresh_status->type == REDIS_REPLY_STRING &&
          strcmp(refresh_status->str, "1") == 0) {
        if (previous_equipment_status->type == REDIS_REPLY_STRING &&
            strcmp(previous_equipment_status->str,
                   equipment_status->str) == 0) {
          if ((refresh_time + seconds_refresh_cutoff) <= current_time) {
            set_command = redisCommand(context, "SET status_refresh 0");
            if (set_command == NULL) {
              sd_journal_send("MESSAGE=%s",
                              "Failed to set status_refresh.",
                              "PRIORITY=%i",
                              LOG_ERR,
                              NULL);
              continue;
            } else {
              freeReplyObject(set_command);
            }
            // TODO: GNSS query
            time_print = localtime(&current_time);
            strftime(time_buffer,
                     sizeof(time_buffer),
                     "%Y-%m-%d-%H:%M:%S",
                     time_print);
            buffer_size = strlen(time_buffer)
              + strlen(equipment_status->str)
              + strlen(location)
              + 3;
            message = (char *) malloc(buffer_size * sizeof(char));
            memset(message, 0, buffer_size * sizeof(char));
            strcpy(message, time_buffer);
            strcat(message, " ");
            strcat(message, equipment_status->str);
            strcat(message, " ");
            strcat(message, location);
            set_command = redisCommand(context, "RPUSH messages %s", message);
            if (set_command == NULL) {
              sd_journal_send(
                "MESSAGE=%s",
                "Failed to get redis response while pushing update.",
                "PRIORITY=%i",
                LOG_ERR,
                NULL
              );
              continue;
            } else {
              freeReplyObject(set_command);
            }
            free(message);
            refresh_time = current_time;
          }
        } else {
          set_command = redisCommand(context,
                                     "SET previous_equipment_status %s",
                                     equipment_status->str);
          if (set_command == NULL) {
            sd_journal_send("MESSAGE=%s",
                            "Failed to set previous_equipment_status.",
                            "PRIORITY=%i",
                            LOG_ERR,
                            NULL);
            continue;
          } else {
            freeReplyObject(set_command);
          }
          refresh_time = current_time;
        }
      } else {
        if ((refresh_time + seconds_location_period) <= current_time) {
          // TODO: GNSS query
          time_print = localtime(&current_time);
          strftime(time_buffer,
                   sizeof(time_buffer),
                   "%Y-%m-%d-%H:%M:%S",
                   time_print);
          buffer_size = strlen(time_buffer)
            + strlen(equipment_status->str)
            + strlen(location)
            + 3;
          message = (char *) malloc(buffer_size * sizeof(char));
          memset(message, 0, buffer_size * sizeof(char));
          strcpy(message, time_buffer);
          strcat(message, " ");
          strcat(message, equipment_status->str);
          strcat(message, " ");
          strcat(message, location);
          set_command = redisCommand(context, "RPUSH messages %s", message);
          if (set_command == NULL) {
            sd_journal_send(
              "MESSAGE=%s",
              "Failed to get redis response while pushing location update.",
              "PRIORITY=%i",
              LOG_ERR,
              NULL
            );
            continue;
          } else {
            freeReplyObject(set_command);
          }
          free(message);
          refresh_time = current_time;
        }
      }
    }

    freeReplyObject(equipment_status);
    freeReplyObject(previous_equipment_status);
    freeReplyObject(refresh_status);
    freeReplyObject(status_queue);
  }
  redisFree(context);
  return 0;
}
