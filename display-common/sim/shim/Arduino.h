// Minimal Arduino.h shim for the host-side display preview (sim/).
//
// This is NOT a general Arduino emulator — it provides only the handful of
// symbols that config.h / display.cpp / weather_fetcher.h and Adafruit_GFX touch
// when compiled on the Mac. Real firmware never sees this file (it's only on the
// include path under -DHOST_SIM via scripts/preview.sh).
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>

#define ARDUINO 100
#ifndef PROGMEM
#define PROGMEM
#endif
#define PSTR(s) (s)
#define F(s) (s)

typedef uint8_t  byte;
typedef bool     boolean;

class __FlashStringHelper;  // F()/flash-string overloads reference this (unused on host)

#ifndef radians
#define radians(deg) ((deg) * 0.017453292519943295)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * 57.29577951308232)
#endif
#ifndef sq
#define sq(x) ((x) * (x))
#endif

// Board macros config.h references for pin constants (unused in the preview).
#ifndef TFT_BACKLITE
#define TFT_BACKLITE 0
#endif
#ifndef TFT_I2C_POWER
#define TFT_I2C_POWER 0
#endif

// --- Arduino String, backed by std::string (subset used by the firmware) ---
class String {
  public:
    String() {}
    String(const char* s) : s_(s ? s : "") {}
    String(char c) : s_(1, c) {}
    String(const std::string& s) : s_(s) {}
    String(int v)      { char b[16]; snprintf(b, sizeof(b), "%d", v); s_ = b; }
    String(unsigned v) { char b[16]; snprintf(b, sizeof(b), "%u", v); s_ = b; }

    unsigned length() const { return (unsigned)s_.size(); }
    const char* c_str() const { return s_.c_str(); }
    char charAt(unsigned i) const { return i < s_.size() ? s_[i] : 0; }
    char operator[](unsigned i) const { return charAt(i); }
    void setCharAt(unsigned i, char c) { if (i < s_.size()) s_[i] = c; }

    int indexOf(char c, unsigned from = 0) const {
        size_t p = s_.find(c, from);
        return p == std::string::npos ? -1 : (int)p;
    }
    String substring(unsigned from) const {
        return from <= s_.size() ? String(s_.substr(from)) : String();
    }
    String substring(unsigned from, unsigned to) const {
        if (from > s_.size()) return String();
        if (to > s_.size()) to = (unsigned)s_.size();
        if (to < from) to = from;
        return String(s_.substr(from, to - from));
    }

    String& operator+=(const String& o) { s_ += o.s_; return *this; }
    String& operator+=(const char* o)   { s_ += (o ? o : ""); return *this; }
    String& operator+=(char c)          { s_ += c; return *this; }

    friend String operator+(const String& a, const String& b) { return String(a.s_ + b.s_); }
    friend String operator+(const String& a, const char* b)   { return String(a.s_ + (b ? b : "")); }
    friend String operator+(const char* a, const String& b)   { return String(std::string(a ? a : "") + b.s_); }

  private:
    std::string s_;
};

// --- Minimal IPAddress (display.h signatures reference it) ---
class IPAddress {
  public:
    IPAddress() {}
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : a_(a), b_(b), c_(c), d_(d) {}
    String toString() const {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u", a_, b_, c_, d_);
        return String(buf);
    }
  private:
    uint8_t a_ = 0, b_ = 0, c_ = 0, d_ = 0;
};

// GFX/firmware occasionally use these; provide host equivalents.
#ifndef min
template <class T> static inline T min(T a, T b) { return a < b ? a : b; }
#endif
#ifndef max
template <class T> static inline T max(T a, T b) { return a > b ? a : b; }
#endif

#include "Print.h"
