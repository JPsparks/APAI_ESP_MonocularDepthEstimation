
#include "logger.h"

String resolve_log_level(custom_log_level to_resolve){
    String to_ret = "[";
    switch (to_resolve)
    {
    case INFO:
        to_ret += "I";
        break;
    case WARNING:
        to_ret += "W";
        break;
    case ERROR:
        to_ret += "E";
        break;
    
    default:
        to_ret += " ";
        break;
    }
    to_ret += "] - ";
    return to_ret;
}


void log(String to_log, custom_log_level level = INFO, bool can_print) {
    #ifdef DEBUG_LOG
    if (can_print){
        String prefix = resolve_log_level(level);
        Serial.println(prefix + to_log);
    }
    #endif
}

void default_log(String to_log, bool can_print){
    log(to_log, INFO, can_print);
}

void default_log(String to_log){
    log(to_log, INFO, true);
}
