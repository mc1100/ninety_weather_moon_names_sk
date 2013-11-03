#ifndef CONFIG_H
#define CONFIG_H

#define DAY_NAME_LANGUAGE			DAY_NAME_SLOVAK			// Valid values: DAY_NAME_SLOVAK
#define MOONPHASE_NAME_LANGUAGE		MOONPHASE_TEXT_SLOVAK	// Valid values: MOONPHASE_TEXT_SLOVAK
#define TRANSLATION_DAY_WEEK		"D%j/T%V"				// Translation for the calendar week (e.g. "CW%V")
	
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

	
// ---- Constants for all available languages ----------------------------------------

const char *DAY_NAME_SLOVAK[] = {
	"NE",
	"PO",
	"UT",
	"ST",
	"ŠT",
	"PI",
	"SO"
};

const char *MOONPHASE_TEXT_SLOVAK[] = {
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