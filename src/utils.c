#define PCRE2_CODE_UNIT_WIDTH 8

#include <glib.h>
#include <pcre2.h>
#include <systemd/sd-journal.h>
#include "utils.h"

int get_char_key(const char  *key,
                 char       **value)
{
  int out = 0;
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    return out;
  }
  redisReply *reply = redisCommand(context, "GET %s", key);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error getting key: %s", key, "PRIORITY=%i", LOG_ERR, NULL);
    redisFree(context);
    return out;
  }
  if (reply->type == REDIS_REPLY_STRING) {
    g_free(*value);
    int buffer_size = strlen(reply->str);
    *value = (char *) g_malloc(buffer_size * sizeof(char));
    strcpy(*value, reply->str);
    out = 1;
  }
  freeReplyObject(reply);
  redisFree(context);
  return out;
}

int get_int_key(const char *key,
                int        *value)
{
  int out = 0;
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    return out;
  }
  redisReply *reply = redisCommand(context, "GET %s", key);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error getting key: %s", key, "PRIORITY=%i", LOG_ERR, NULL);
    redisFree(context);
    return out;
  }
  if (reply->type == REDIS_REPLY_STRING) {
    *value = atoi(reply->str);
    out = 1;
  }
  freeReplyObject(reply);
  redisFree(context);
  return out;
}

int push_message(redisContext *context,
                 const char   *message)
{
  redisReply *reply = redisCommand(context, "RPUSH messages %s", message);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error pushing message: %s", message, "PRIORITY=%i", LOG_ERR, NULL);
    return 0;
  }
  freeReplyObject(reply);
  return 1;
}

int redis_cmd(const char *format,
              ...)
{
  int out = 0;
  va_list args;
  va_start(args, format);
  int len = vsnprintf(NULL, 0, format, args) + 1;
  va_end(args);
  char *cmd = malloc(len);
  va_start(args, format);
  vsnprintf(cmd, len, format, args);
  va_end(args);
  redisContext *context = redisConnect("localhost", 6379);
  if (context == NULL || context->err) {
    if (context) {
      sd_journal_send("MESSAGE=Connection error: %s", context->errstr, "PRIORITY=%i", LOG_ERR, NULL);
      redisFree(context);
    } else
      sd_journal_send("MESSAGE=%s", "Connection error: can't allocate redis context", "PRIORITY=%i", LOG_ERR, NULL);
    free(cmd);
    return out;
  }
  redisReply *reply = redisCommand(context, cmd);
  if (reply == NULL) {
    sd_journal_send("MESSAGE=Error executing Redis command: %s", cmd, "PRIORITY=%i", LOG_ERR, NULL);
  } else {
    if (reply->type != REDIS_REPLY_ERROR)
      out = 1;
    freeReplyObject(reply);
  }
  free(cmd);
  redisFree(context);
  return out;
}

int send_equipment_status(redisContext *context)
{
  int output = 0;
  redisReply *status_queue = NULL;
  status_queue = redisCommand(context, "LRANGE messages 0 -1");
  if (status_queue == NULL) {
    sd_journal_send("MESSAGE=%s", "Failed to get status queue while attempting to send message.", "PRIORITY=%i", LOG_ERR, NULL);
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
      for (counter = 0; counter < status_queue->elements; counter++)
        total_characters += strlen(status_queue->element[counter]->str);
      buffer_size = total_characters + (status_queue->elements * 2);
      messages = (char *) malloc(buffer_size * sizeof(char));
      memset(messages, 0, buffer_size * sizeof(char));
      strcpy(messages, status_queue->element[0]->str);
      strcat(messages, "\n");
      capture_pattern(status_queue->element[0]->str, captured_datetime, captured_equipment_status, captured_location);
      for (counter = 1; counter < status_queue->elements; counter++) {
        capture_pattern(status_queue->element[counter]->str, new_captured_datetime, new_captured_equipment_status, new_captured_location);
        str_difference(captured_datetime, new_captured_datetime, datetime_substr);
        strcat(messages, datetime_substr);
        strcat(messages, " ");
        if (strcmp(captured_equipment_status, new_captured_equipment_status) != 0) {
          strcat(messages, new_captured_equipment_status);
          strcat(messages, " ");
        }
        strcat(messages, new_captured_location);
        strcat(messages, "\n");
        strcpy(captured_datetime, new_captured_datetime);
        strcpy(captured_equipment_status, new_captured_equipment_status);
      }
      send_sms("All messages:");
      // TODO: return the status of sending messages and delete cache if sending
      // is success.
      send_sms(messages);
      free(messages);

      if (!redis_cmd("DEL messages"))
        output = 3;
    } else {
      output = 2;
    }
    freeReplyObject(status_queue);
  }
  return output;
}

void capture_pattern(const char *source,
                     char       *datetime,
                     char       *status,
                     char       *location)
{
  const PCRE2_SPTR pattern = (PCRE2_SPTR) "^(\\d{4}-\\d{2}-\\d{2}-\\d{2}:\\d{2}:\\d{2})\\s+([a-z\\-]+)\\s+(LOCATION).*";
  int error_code;
  int matches;
  pcre2_code *re;
  PCRE2_SIZE error_offset;
  re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL);
  if (re == NULL) {
    PCRE2_UCHAR buffer[256];
    pcre2_get_error_message(error_code, buffer, sizeof(buffer));
    sd_journal_send("MESSAGE=PCRE2 compilation failed at offset %d: %s", (int) error_offset, buffer, "PRIORITY=%i", LOG_ERR, NULL);
    return;
  }
  pcre2_match_data *match_data;
  match_data = pcre2_match_data_create_from_pattern(re, NULL);
  matches = pcre2_match(re, (PCRE2_SPTR) source, strlen(source), 0, 0, match_data, NULL);
  if (matches < 0) {
    switch(matches) {
      case PCRE2_ERROR_NOMATCH:
        sd_journal_send("MESSAGE=%s", "PCRE2 no matches", "PRIORITY=%i", LOG_ERR, NULL);
        break;
      default:
        sd_journal_send("MESSAGE=PCRE2 matching error %d", matches, "PRIORITY=%i", LOG_ERR, NULL);
        break;
    }
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    return;
  }
  const PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
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

void send_sms(const char *format,
              ...)
{
  va_list args;
  va_start(args, format);
  int len = vsnprintf(NULL, 0, format, args) + 1;
  va_end(args);
  char *text = g_malloc(len);
  va_start(args, format);
  vsnprintf(text, len, format, args);
  va_end(args);
  // TODO: send actual SMS
  printf("%s\n", text);
  g_free(text);
}

void set_system_time(void)
{
  // TODO: set system time
  printf("System time set.\n");
}

void str_copy(char       **destination,
              const char  *source)
{
  g_free(*destination);
  *destination = g_malloc(strlen(source) * sizeof(char));
  strcpy(*destination, source);
}

void str_difference(const char *old,
                    const char *new,
                    char       *holder)
{
  int counter = 0;
  int idx;
  holder[0] = '\0';
  while (1) {
    if (old[counter] != new[counter])
      break;
    counter += 1;
  }
  for (idx = 0; counter < strlen(new); idx++) {
    holder[idx] = new[counter];
    counter += 1;
  }
  holder[idx] = '\0';
}
