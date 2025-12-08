#ifndef __PW_APAI_LOGGER_H
#define __PW_APAI_LOGGER_H

#include "Arduino.h"
#include "../config.h"

typedef enum {
    INFO,
    WARNING,
    ERROR,
    NONE, 
} custom_log_level;

String resolve_log_level(custom_log_level to_resolve);

void log(String to_log, custom_log_level level, bool can_print = true);

void default_log(String to_log, bool can_print);

void default_log(String to_log);

#endif 