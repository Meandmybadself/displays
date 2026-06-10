#include "Arduino.h"

size_t Print::print(const String& s) {
    return write((const uint8_t*)s.c_str(), s.length());
}
