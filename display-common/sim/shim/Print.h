// Minimal Arduino Print base for the host preview. Adafruit_GFX derives from
// Print and overrides write(uint8_t); the print() helpers below fan out to it,
// so GFX's glyph-drawing path runs unchanged on the host.
#pragma once

#include <cstddef>
#include <cstdint>

class String;  // from Arduino.h

class Print {
  public:
    virtual ~Print() {}
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* buf, size_t len) {
        size_t n = 0;
        while (len--) n += write(*buf++);
        return n;
    }

    size_t print(char c)         { return write((uint8_t)c); }
    size_t print(const char* s)  { return s ? write((const uint8_t*)s, __builtin_strlen(s)) : 0; }
    size_t print(const String& s);  // defined in Print.cpp (needs full String)
};
