#include "trinet.h"
#include <stdio.h>

void Log(LogLevel level, const char *reason, const char *text, ...) {
    char *level_text = "INVALID_LOG";
    switch (level) {
        case LOG_DEBUG: 
            level_text = "DEBUG";
            if (quietLogs) return;
            break;
        case LOG_INFO: level_text = "INFO"; break;
        case LOG_WARNING: level_text = "WARNING"; break;
        case LOG_ERROR: level_text = "ERROR"; break;
    }
    fprintf(stderr, "[%s] %s: %s\n", level_text, reason, text);
}