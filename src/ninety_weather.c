#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pebble.h"

#include "config.h"

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

enum WeatherKey {
    WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
    WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
    SUNRISE_KEY = 0x2,              // TUPLE_CSTRING
    SUNSET_KEY = 0x3                // TUPLE_CSTRING
};

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
        RESOURCE_ID_IMAGE_NO_WEATHER,
        RESOURCE_ID_IMAGE_WEATHER_01D,
        RESOURCE_ID_IMAGE_WEATHER_01N,
        RESOURCE_ID_IMAGE_WEATHER_02D,
        RESOURCE_ID_IMAGE_WEATHER_02N,
        RESOURCE_ID_IMAGE_WEATHER_03,
        RESOURCE_ID_IMAGE_WEATHER_03,
        RESOURCE_ID_IMAGE_WEATHER_04,
        RESOURCE_ID_IMAGE_WEATHER_04,
        RESOURCE_ID_IMAGE_WEATHER_09D,
        RESOURCE_ID_IMAGE_WEATHER_09N,
        RESOURCE_ID_IMAGE_WEATHER_10D,
        RESOURCE_ID_IMAGE_WEATHER_10N,
        RESOURCE_ID_IMAGE_WEATHER_11D,
        RESOURCE_ID_IMAGE_WEATHER_11N,
        RESOURCE_ID_IMAGE_WEATHER_13D,
        RESOURCE_ID_IMAGE_WEATHER_13N,
        RESOURCE_ID_IMAGE_WEATHER_50D,
        RESOURCE_ID_IMAGE_WEATHER_50N,
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

double frac(double a) {
    return a - floor(a);
}

double fmod(double a, double b) {
    double c = frac(fabs(a / b)) * fabs(b);
    return (a < 0) ? -c : c;
}

/*
 * Calculates the moon phase (0-7), accurate to 1 segment.
 * 0 = > new moon.
 * 4 => full moon.
 */
int get_moon_phase() {
    return (int) (round(fmod((time(NULL) - 592500) / 86400.d, 29.53058868) * 8 / 29.53058868d)) & 7;
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
            int moonphase = get_moon_phase();
            set_container_image(&moon_phase_image, moon_phase_layer, MOON_IMAGE_RESOURCE_IDS[moonphase], GPoint(61, 143));
            text_layer_set_text(moon_layer, moon_phase_text[moonphase]);

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

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch (key) {
    case WEATHER_ICON_KEY:
        set_container_image(&weather_image, weather_layer, WEATHER_IMAGE_RESOURCE_IDS[new_tuple->value->uint8], GPoint(4, 5));
        break;
    case WEATHER_TEMPERATURE_KEY:
        text_layer_set_text(text_temperature_layer, new_tuple->value->cstring);
        break;
    case SUNRISE_KEY:
        text_layer_set_text(text_sunrise_layer, new_tuple->value->cstring);
        break;
    case SUNSET_KEY:
        text_layer_set_text(text_sunset_layer, new_tuple->value->cstring);
        break;
    }
}

static void window_load(Window *window) {
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

    // Sunset text
    text_sunset_layer = text_layer_create(GRect(110, 152, 29, 16));
    text_layer_set_text_color(text_sunset_layer, GColorWhite);
    text_layer_set_background_color(text_sunset_layer, GColorClear);
    text_layer_set_font(text_sunset_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(text_sunset_layer));

    // Avoids a blank screen on watch start.
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);

    update_display(tick_time);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    Tuplet initial_values[] = {
            TupletInteger(WEATHER_ICON_KEY, (uint8_t) 0),
            TupletCString(WEATHER_TEMPERATURE_KEY, "- °"),
            TupletCString(SUNRISE_KEY, "--:--"),
            TupletCString(SUNSET_KEY, "--:--")
    };

    app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
            sync_tuple_changed_callback, sync_error_callback, NULL);
}

static void window_unload(Window *window) {
    app_sync_deinit(&sync);

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

    window_stack_push(window, true);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
