#include "pebble.h"
#include "stdlib.h"
#include "string.h"
#include "config.h"
#include "my_math.h"
#include "suncalc.h"
//#include "http.h"
//#include "sync.h"
#include "util.h"

static Window *window = NULL;

static GBitmap *background_image = NULL;
static BitmapLayer *background_layer = NULL;

static GBitmap *time_format_image = NULL;
static BitmapLayer *time_format_layer = NULL;

static GBitmap *day_name_image = NULL;
static BitmapLayer *day_name_layer = NULL;

static GBitmap *weather_image = NULL;
static BitmapLayer *weather_layer = NULL;

static GBitmap *moon_phase_image = NULL;
static BitmapLayer *moon_phase_layer = NULL;

#define TOTAL_DATE_DIGITS 6
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS] = { NULL, NULL, NULL, NULL, NULL, NULL };
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS] = { NULL, NULL, NULL, NULL, NULL, NULL };

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS] = { NULL, NULL, NULL, NULL };
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS] = { NULL, NULL, NULL, NULL };

static TextLayer *nd_layer = NULL;
static TextLayer *cw_layer = NULL;
static TextLayer *calls_layer = NULL;
static TextLayer *text_temperature_layer = NULL;
static TextLayer *sms_layer = NULL;
static TextLayer *moon_layer = NULL;
static TextLayer *text_sunrise_layer = NULL;
static TextLayer *text_sunset_layer = NULL;

static AppSync sync;
static uint8_t sync_buffer[64];
static AppSync sync;
static uint8_t sync_buffer[64];

enum WeatherKey {
    WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
    WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
    WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

static float our_latitude;
static float our_longitude;
static float our_timezone;
//static bool located = false;
//static bool time_received = false;
static unsigned short the_last_hour = 25;

const int const BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
        RESOURCE_ID_IMAGE_NUM_0,
        RESOURCE_ID_IMAGE_NUM_1,
        RESOURCE_ID_IMAGE_NUM_2,
        RESOURCE_ID_IMAGE_NUM_3,
        RESOURCE_ID_IMAGE_NUM_4,
        RESOURCE_ID_IMAGE_NUM_5,
        RESOURCE_ID_IMAGE_NUM_6,
        RESOURCE_ID_IMAGE_NUM_7,
        RESOURCE_ID_IMAGE_NUM_8,
        RESOURCE_ID_IMAGE_NUM_9,
        RESOURCE_ID_IMAGE_NUM_X
};

const int const DATENUM_IMAGE_RESOURCE_IDS[] = {
        RESOURCE_ID_IMAGE_DATENUM_0,
        RESOURCE_ID_IMAGE_DATENUM_1,
        RESOURCE_ID_IMAGE_DATENUM_2,
        RESOURCE_ID_IMAGE_DATENUM_3,
        RESOURCE_ID_IMAGE_DATENUM_4,
        RESOURCE_ID_IMAGE_DATENUM_5,
        RESOURCE_ID_IMAGE_DATENUM_6,
        RESOURCE_ID_IMAGE_DATENUM_7,
        RESOURCE_ID_IMAGE_DATENUM_8,
        RESOURCE_ID_IMAGE_DATENUM_9
};

const int const DAY_NAME_IMAGE_RESOURCE_IDS[] = {
        RESOURCE_ID_IMAGE_DAY_NAME_SUN,
        RESOURCE_ID_IMAGE_DAY_NAME_MON,
        RESOURCE_ID_IMAGE_DAY_NAME_TUE,
        RESOURCE_ID_IMAGE_DAY_NAME_WED,
        RESOURCE_ID_IMAGE_DAY_NAME_THU,
        RESOURCE_ID_IMAGE_DAY_NAME_FRI,
        RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

const int const WEATHER_IMAGE_RESOURCE_IDS[] = {
        RESOURCE_ID_IMAGE_CLEAR_DAY,
        RESOURCE_ID_IMAGE_CLEAR_NIGHT,
        RESOURCE_ID_IMAGE_RAIN,
        RESOURCE_ID_IMAGE_SNOW,
        RESOURCE_ID_IMAGE_SLEET,
        RESOURCE_ID_IMAGE_WIND,
        RESOURCE_ID_IMAGE_FOG,
        RESOURCE_ID_IMAGE_CLOUDY,
        RESOURCE_ID_IMAGE_PARTLY_CLOUDY_DAY,
        RESOURCE_ID_IMAGE_PARTLY_CLOUDY_NIGHT,
        RESOURCE_ID_IMAGE_NO_WEATHER,
        RESOURCE_ID_IMAGE_NO_BLUETOOTH
};

const int const MOON_IMAGE_RESOURCE_IDS[] = {
        RESOURCE_ID_IMAGE_MOON_0,
        RESOURCE_ID_IMAGE_MOON_1,
        RESOURCE_ID_IMAGE_MOON_2,
        RESOURCE_ID_IMAGE_MOON_3,
        RESOURCE_ID_IMAGE_MOON_4,
        RESOURCE_ID_IMAGE_MOON_5,
        RESOURCE_ID_IMAGE_MOON_6,
        RESOURCE_ID_IMAGE_MOON_7
};

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
    GBitmap *old_image = *bmp_image;

    *bmp_image = gbitmap_create_with_resource(resource_id);
    GRect frame = (GRect) {
            .origin = origin,
            .size = (*bmp_image)->bounds.size
    };
    bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
    layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

    if (old_image) {
        gbitmap_destroy(old_image);
    }
}

static unsigned short get_display_hour(unsigned short hour) {
    if (clock_is_24h_style()) {
        return hour;
    }

    unsigned short display_hour = hour % 12;

    // converts "0" to "12"
    return display_hour ? display_hour : 12;
}

/*
 * Calculates the moon phase (0-7), accurate to 1 segment.
 * 0 = > new moon.
 * 4 => full moon.
 */
int moon_phase(int y, int m, int d) {
    int c, e;
    double jd;
    int b;

    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25d * y;
    e = 30.6d * m;
    jd = c + e + d - 694039.09d; /* jd is total days elapsed */
    jd /= 29.53d; /* divide by the moon cycle (29.53 days) */
    b = jd; /* int(jd) -> b, take integer part of jd */
    jd -= b; /* subtract integer part to leave fractional part of original jd */
    b = jd * 8 + 0.5d; /* scale fraction from 0-8 and round by adding 0.5 */
    b = b & 7; /* 0 and 8 are the same so turn 8 into 0 */

    return b;
}

void updateSunsetSunrise() {
    static char sunrise_text[6];
    static char sunset_text[6];

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);

    char *time_format;

    if (clock_is_24h_style()) {
        time_format = "%R";
    } else {
        time_format = "%I:%M";
    }

    float sunriseTime = calcSun(tick_time->tm_year, tick_time->tm_mon + 1, tick_time->tm_mday, our_latitude, our_longitude, 0,
    ZENITH_OFFICIAL) + our_timezone;
    float sunsetTime = calcSun(tick_time->tm_year, tick_time->tm_mon + 1, tick_time->tm_mday, our_latitude, our_longitude, 1,
    ZENITH_OFFICIAL) + our_timezone;

    tick_time->tm_min = (int) (60 * (sunriseTime - ((int) (sunriseTime))));
    tick_time->tm_hour = (int) sunriseTime;
    strftime(sunrise_text, sizeof(sunrise_text), time_format, tick_time);
    text_layer_set_text(text_sunrise_layer, sunrise_text);

    tick_time->tm_min = (int) (60 * (sunsetTime - ((int) (sunsetTime))));
    tick_time->tm_hour = (int) sunsetTime;
    strftime(sunset_text, sizeof(sunset_text), time_format, tick_time);
    text_layer_set_text(text_sunset_layer, sunset_text);
}
/*
void request_weather() {
    // Build the HTTP request
    DictionaryIterator *body;
    HTTPResult result = http_out_get("http://pwdb.kathar.in/pebble/weather3.php", WEATHER_HTTP_COOKIE, &body);
    if (result != HTTP_OK) {
        return;
    }

    dict_write_int32(body, WEATHER_KEY_LATITUDE, (int) (our_latitude * 10000));
    dict_write_int32(body, WEATHER_KEY_LONGITUDE, (int) (our_longitude * 10000));
    dict_write_cstring(body, WEATHER_KEY_UNIT_SYSTEM, UNIT_SYSTEM);

    // Send it.
    http_out_send();
}

void failed(int32_t cookie, int http_status, void* context) {
    if (cookie == WEATHER_HTTP_COOKIE) {
        set_container_image(&weather_image, weather_layer, WEATHER_IMAGE_RESOURCE_IDS[11], GPoint(4, 5));
        text_layer_set_text(text_temperature_layer, "");
    }
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
    Tuple* data_tuple = dict_find(received, WEATHER_KEY_CURRENT);
    if (data_tuple) {
        uint16_t value = data_tuple->value->int16;
        uint8_t icon = value >> 11;
        if (icon > 10) {
            icon = 10;
        }
        set_container_image(&weather_image, weather_layer, WEATHER_IMAGE_RESOURCE_IDS[icon], GPoint(4, 5));
        int16_t temp = value & 0x3ff;
        if (value & 0x400) {
            temp = -temp;
        }
        static char temp_text[8];
        memcpy(temp_text, itoa(temp), 4);
        int degree_pos = strlen(temp_text);
        memcpy(&temp_text[degree_pos], " °", 4);
        text_layer_set_text(text_temperature_layer, temp_text);
    } else if (cookie == WEATHER_HTTP_COOKIE) {
        set_container_image(&weather_image, weather_layer, WEATHER_IMAGE_RESOURCE_IDS[10], GPoint(4, 5));
        text_layer_set_text(text_temperature_layer, "");
    }
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
    our_latitude = latitude;
    our_longitude = longitude;
    located = true;

    if (time_received) {
        updateSunsetSunrise();
    }
    request_weather();
}

void reconnect(void* context) {
    located = false;
    time_received = false;
    request_phone_state();
}

bool read_state_data(DictionaryIterator* received, struct Data* d) {
    bool has_data = false;
    Tuple* tuple = dict_read_first(received);
    if (!tuple) {
        return false;
    }

    do {
        switch (tuple->key) {
        case TUPLE_MISSED_CALLS:
            d->missed = tuple->value->uint8;

            static char temp_calls[5];
            memcpy(temp_calls, itoa(tuple->value->uint8), 4);
            text_layer_set_text(calls_layer, temp_calls);

            has_data = true;
            break;

        case TUPLE_UNREAD_SMS:
            d->unread = tuple->value->uint8;

            static char temp_sms[5];
            memcpy(temp_sms, itoa(tuple->value->uint8), 4);
            text_layer_set_text(sms_layer, temp_sms);

            has_data = true;
            break;
        }
    } while ((tuple = dict_read_next(received)));

    return has_data;
}

void app_received_msg(DictionaryIterator* received, void* context) {
    if (read_state_data(received, &data)) {
        if (!located) {
            http_location_request();
        }
        if (!time_received) {
            http_time_request();
        }
    }
}

bool register_callbacks() {
    if (callbacks_registered) {
        if (app_message_deregister_callbacks(&app_callbacks) == APP_MSG_OK) {
            callbacks_registered = false;
        }
    }
    if (!callbacks_registered) {
        app_callbacks = (AppMessageCallbacksNode ) { .callbacks = { .in_received = app_received_msg } };
        if (app_message_register_callbacks(&app_callbacks) == APP_MSG_OK) {
            callbacks_registered = true;
        }
    }

    return callbacks_registered;
}

void receivedtime(int32_t utc_offset_seconds, bool is_dst, uint32_t unixtime, const char* tz_name, void* context) {
    our_timezone = (utc_offset_seconds / 3600.0f);
    time_received = true;

    if (located) {
        updateSunsetSunrise();
    }
}
*/

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch (key) {
    case WEATHER_ICON_KEY:
        set_container_image(&weather_image, weather_layer, WEATHER_IMAGE_RESOURCE_IDS[new_tuple->value->uint8], GPoint(4, 5));
        break;
    case WEATHER_TEMPERATURE_KEY:
        // App Sync keeps new_tuple in sync_buffer, so we may use it directly
        text_layer_set_text(text_temperature_layer, new_tuple->value->cstring);
        break;
    case WEATHER_CITY_KEY:
        break;
    }
}

void update_display(struct tm *current_time) {
    unsigned short display_hour = get_display_hour(current_time->tm_hour);

    // Minute
    set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min / 10],
            GPoint(80, 94));
    set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min % 10],
            GPoint(111, 94));

    if (the_last_hour != display_hour) {
        int h = display_hour / 10;
        if (!clock_is_24h_style()) {
            if (the_last_hour == 25 || display_hour == 12) {
                set_container_image(&time_format_image, time_format_layer,
                        (current_time->tm_hour < 12) ? RESOURCE_ID_IMAGE_AM_MODE : RESOURCE_ID_IMAGE_PM_MODE, GPoint(118, 140));
            }
            if (!h) {
                h = 10;
            }
        }

        // Hour
        set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[h], GPoint(4, 94));
        set_container_image(&time_digits_images[1], time_digits_layers[1],
                BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour % 10], GPoint(37, 94));

        if (the_last_hour == 25 || !current_time->tm_hour) {
            // Day of week
            set_container_image(&day_name_image, day_name_layer,
                    DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(4, 71));

            // Day
            set_container_image(&date_digits_images[0], date_digits_layers[0],
                    DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday / 10], GPoint(55, 71));
            set_container_image(&date_digits_images[1], date_digits_layers[1],
                    DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday % 10], GPoint(68, 71));

            // Month
            set_container_image(&date_digits_images[2], date_digits_layers[2],
                    DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) / 10], GPoint(87, 71));
            set_container_image(&date_digits_images[3], date_digits_layers[3],
                    DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) % 10], GPoint(100, 71));

            // Year
            set_container_image(&date_digits_images[4], date_digits_layers[4],
                    DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_year % 100) / 10], GPoint(115, 71));
            set_container_image(&date_digits_images[5], date_digits_layers[5],
                    DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_year % 100) % 10], GPoint(128, 71));

            // moon phase
            int moonphase_number;
            moonphase_number = moon_phase(current_time->tm_year + 1900, current_time->tm_mon, current_time->tm_mday);

            set_container_image(&moon_phase_image, moon_phase_layer, MOON_IMAGE_RESOURCE_IDS[moonphase_number], GPoint(61, 143));
            text_layer_set_text(moon_layer, moon_phase_text[moonphase_number]);

            // nameday
            text_layer_set_text(nd_layer, nameday_text[current_time->tm_mon][current_time->tm_mday - 1]);

            // day & calendar week
            static char cw_text[9];
            strftime(cw_text, sizeof(cw_text), "D%j/T%V", current_time);
            text_layer_set_text(cw_layer, cw_text);
        }

        the_last_hour = display_hour;
    }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    update_display(tick_time);

    /*
    if (!(tick_time->tm_min % 15)) {
        located = false;
        time_received = false;
    }
    if (data.link_status == LinkStatusUnknown) {
        request_phone_state();
    } else {
        if (!located) {
            http_location_request();
        }
        if (!time_received) {
            http_time_request();
        }
    }
    */
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) {
  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 0),
    TupletCString(WEATHER_TEMPERATURE_KEY, "-18 °"),
    TupletCString(WEATHER_CITY_KEY, "St Pebblesburg"),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

  send_cmd();
}

static void window_unload(Window *window) {
  app_sync_deinit(&sync);
}

static void init(void) {
    window = window_create();
    window_set_background_color(window, GColorBlack);
    window_set_fullscreen(window, true);
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload
    });

    app_message_open(64, 64);
    window_stack_push(window, true /* Animated */);
    Layer *window_layer = window_get_root_layer(window);

    background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    background_layer = bitmap_layer_create(layer_get_frame(window_layer));
    bitmap_layer_set_bitmap(background_layer, background_image);
    layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

    if (clock_is_24h_style()) {
        time_format_image = gbitmap_create_with_resource((clock_is_24h_style()) ? RESOURCE_ID_IMAGE_24_HOUR_MODE : RESOURCE_ID_IMAGE_AM_MODE);
        GRect frame = (GRect) {
                .origin = { .x = 118, .y = 140 },
                .size = time_format_image->bounds.size
        };
        time_format_layer = bitmap_layer_create(frame);
        bitmap_layer_set_bitmap(time_format_layer, time_format_image);
        layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));
    } else {
        time_format_layer = bitmap_layer_create(GRectZero);
        layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));
    }

    /*
    day_name_layer = bitmap_layer_create(GRectZero);
    layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));

    weather_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NO_WEATHER);
    frame = (GRect) {
            .origin = { .x = 4, .y = 5 },
            .size = weather_image->bounds.size
    };
    weather_layer = bitmap_layer_create(frame);
    bitmap_layer_set_bitmap(weather_layer, weather_image);
    layer_add_child(window_layer, bitmap_layer_get_layer(weather_layer));
    */

    day_name_layer = bitmap_layer_create(GRectZero);
    layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));

    weather_layer = bitmap_layer_create(GRectZero);
    layer_add_child(window_layer, bitmap_layer_get_layer(weather_layer));

    moon_phase_layer = bitmap_layer_create(GRectZero);
    layer_add_child(window_layer, bitmap_layer_get_layer(moon_phase_layer));

    // Create time and date layers
    for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
        time_digits_layers[i] = bitmap_layer_create(GRectZero);
        layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
    }

    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        date_digits_layers[i] = bitmap_layer_create(GRectZero);
        layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
    }

    // Nameday text
    nd_layer = text_layer_create(GRect(5, 50, 85, 16));
    text_layer_set_text_color(nd_layer, GColorWhite);
    text_layer_set_background_color(nd_layer, GColorClear);
    text_layer_set_font(nd_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(nd_layer));

    // Calendar week text
    cw_layer = text_layer_create(GRect(90, 50, 49, 16));
    text_layer_set_text_color(cw_layer, GColorWhite);
    text_layer_set_background_color(cw_layer, GColorClear);
    text_layer_set_font(cw_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(cw_layer, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(cw_layer));

    // Temperature text
    text_temperature_layer = text_layer_create(GRect(50, 1, 89, 44));
    text_layer_set_text_color(text_temperature_layer, GColorWhite);
    text_layer_set_background_color(text_temperature_layer, GColorClear);
    text_layer_set_font(text_temperature_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_alignment(text_temperature_layer, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(text_temperature_layer));
    text_layer_set_text(text_temperature_layer, "- °");

    // Calls info text
    calls_layer = text_layer_create(GRect(12, 135, 22, 16));
    text_layer_set_text_color(calls_layer, GColorWhite);
    text_layer_set_background_color(calls_layer, GColorClear);
    text_layer_set_font(calls_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(calls_layer));
    text_layer_set_text(calls_layer, "-");

    // SMS info text
    sms_layer = text_layer_create(GRect(41, 135, 22, 16));
    text_layer_set_text_color(sms_layer, GColorWhite);
    text_layer_set_background_color(sms_layer, GColorClear);
    text_layer_set_font(sms_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(sms_layer));
    text_layer_set_text(sms_layer, "-");

    // Moon text
    moon_layer = text_layer_create(GRect(88, 136, 30, 16));
    text_layer_set_text_color(moon_layer, GColorWhite);
    text_layer_set_background_color(moon_layer, GColorClear);
    text_layer_set_font(moon_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(moon_layer));

    // Sunrise text
    text_sunrise_layer = text_layer_create(GRect(7, 152, 29, 16));
    text_layer_set_text_color(text_sunrise_layer, GColorWhite);
    text_layer_set_background_color(text_sunrise_layer, GColorClear);
    text_layer_set_font(text_sunrise_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(text_sunrise_layer));
    text_layer_set_text(text_sunrise_layer, "--:--");

    // Sunset text
    text_sunset_layer = text_layer_create(GRect(110, 152, 29, 16));
    text_layer_set_text_color(text_sunset_layer, GColorWhite);
    text_layer_set_background_color(text_sunset_layer, GColorClear);
    text_layer_set_font(text_sunset_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(text_sunset_layer));
    text_layer_set_text(text_sunset_layer, "--:--");

    // data.link_status = LinkStatusUnknown;

    /*
    // request data refresh on window appear (for example after notification)
    WindowHandlers handlers = { .appear = &request_phone_state };
    window_set_window_handlers(&window, handlers);

    http_register_callbacks((HTTPCallbacks) {
            .failure = failed,
            .success = success,
            .reconnect = reconnect,
            .location = location,
            .time = receivedtime
        },
        (void*) ctx
    );

    register_callbacks();
    */

    // Avoids a blank screen on watch start.
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);

    update_display(tick_time);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    /*
    app_message_open(64, 64);

    Tuplet initial_values[] = {
        TupletInteger(WEATHER_ICON_KEY, (uint8_t) 10),
        TupletCString(WEATHER_TEMPERATURE_KEY, "- °"),
        TupletCString(WEATHER_CITY_KEY, "-")
    };

    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
            sync_tuple_changed_callback, sync_error_callback, NULL);

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    if (iter != NULL) {
        dict_write_end(iter);
        app_message_outbox_send();
    }
    */
}

static void deinit(void) {
    //app_sync_deinit(&sync);
    tick_timer_service_unsubscribe();

    layer_remove_from_parent(text_layer_get_layer(text_sunset_layer));
    text_layer_destroy(text_sunset_layer);

    layer_remove_from_parent(text_layer_get_layer(text_sunrise_layer));
    text_layer_destroy(text_sunrise_layer);

    layer_remove_from_parent(text_layer_get_layer(moon_layer));
    text_layer_destroy(moon_layer);

    layer_remove_from_parent(text_layer_get_layer(sms_layer));
    text_layer_destroy(sms_layer);

    layer_remove_from_parent(text_layer_get_layer(calls_layer));
    text_layer_destroy(calls_layer);

    layer_remove_from_parent(text_layer_get_layer(text_temperature_layer));
    text_layer_destroy(text_temperature_layer);

    layer_remove_from_parent(text_layer_get_layer(cw_layer));
    text_layer_destroy(cw_layer);

    layer_remove_from_parent(text_layer_get_layer(nd_layer));
    text_layer_destroy(nd_layer);

    for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
        layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
        bitmap_layer_destroy(date_digits_layers[i]);
        gbitmap_destroy(date_digits_images[i]);
    }

    for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
        layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
        bitmap_layer_destroy(time_digits_layers[i]);
        gbitmap_destroy(time_digits_images[i]);
    }

    layer_remove_from_parent(bitmap_layer_get_layer(moon_phase_layer));
    bitmap_layer_destroy(moon_phase_layer);
    if (moon_phase_image) {
        gbitmap_destroy(moon_phase_image);
    }

    layer_remove_from_parent(bitmap_layer_get_layer(weather_layer));
    bitmap_layer_destroy(weather_layer);
    if (weather_image) {
        gbitmap_destroy(weather_image);
    }

    layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
    bitmap_layer_destroy(day_name_layer);
    if (day_name_image) {
        gbitmap_destroy(day_name_image);
    }

    layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
    bitmap_layer_destroy(time_format_layer);
    if (time_format_image) {
        gbitmap_destroy(time_format_image);
    }

    layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
    bitmap_layer_destroy(background_layer);
    gbitmap_destroy(background_image);

    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
