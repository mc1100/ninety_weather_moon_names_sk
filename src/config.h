#ifndef CONFIG_H
#define CONFIG_H

#define TRANSLATION_DAY_WEEK "D%j/T%V" // Translation for the day & calendar week (e.g. "DY%j/CW%V")

// "auto" (determined by latitude/longitude)
#define UNIT_SYSTEM "si"

// POST variables
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2
#define WEATHER_KEY_UNIT_SYSTEM 3

// Received variables
#define WEATHER_KEY_CURRENT 1
#define WEATHER_KEY_PRECIPITATION 3

//#define WEATHER_HTTP_COOKIE 1949999771
#define WEATHER_HTTP_COOKIE 1949327671

const char * const moon_phase_text[] = {
        "NOV",
        "NOV+",
        "1. ŠT.",
        "SPL-",
        "SPLN",
        "SPL+",
        "3. ŠT.",
        "NOV-"
};

#endif
