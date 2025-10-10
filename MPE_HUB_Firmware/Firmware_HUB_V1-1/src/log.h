#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include "FS.h"
#include "LITTLEFS.h"

class Logger {
public:
    Logger(const char* filename = "/log.txt");
    bool begin(bool formatOnFail = true);
    void log(const String &data);
    void readAll(Stream &out);
    void checkFS(Stream &out);
    unsigned long lastlogtime = 0;

private:
    const char* _filename;
};

#endif
