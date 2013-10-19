#ifndef CONFIG_H
#define CONFIG_H

#define DAY_NAME_LANGUAGE			DAY_NAME_SLOVAK			// Valid values: DAY_NAME_SLOVAK
#define MOONPHASE_NAME_LANGUAGE		MOONPHASE_TEXT_SLOVAK	// Valid values: MOONPHASE_TEXT_SLOVAK
#define day_month_x					day_month_day_first		// Valid values: day_month_month_first, day_month_day_first
#define TRANSLATION_CW				"D%j/T%V"				// Translation for the calendar week (e.g. "CW%V")
	
// "auto" (determined by latitude/longitude)
#define UNIT_SYSTEM "si"

// POST variables
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2
#define WEATHER_KEY_UNIT_SYSTEM 3
	
// Received variables
#define WEATHER_KEY_ICON 1
#define WEATHER_KEY_TEMPERATURE 2

#define WEATHER_HTTP_COOKIE 1949327671
#define TIME_HTTP_COOKIE 1131038282

	
// ---- Constants for all available languages ----------------------------------------

const int day_month_day_first[] = {
	55,
	87,
	115
};

/*
const int day_month_month_first[] = {
	87,
	55,
	115
};
*/

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