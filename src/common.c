#include <trinet.h>
#include <stdbool.h>

bool quietLogs = false;

void Log(LogLevel level, const char *reason, const char *text, ...) {
    if (quietLogs && (level == LOG_INFO || level == LOG_WARNING)) return;

    va_list args;
    va_start(args, text);

    switch (level) {
        case LOG_INFO:    printf("[INFO]: "); break;
        case LOG_WARNING: printf("[WARNING]: "); break;
        case LOG_ERROR:   printf("[ERROR]: "); break;
        case LOG_DEBUG:   printf("[DEBUG]: "); break;
    }

    printf("%s: ", reason);
    vprintf(text, args);
    printf("\n");

    va_end(args);
}