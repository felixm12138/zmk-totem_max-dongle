#include "battery_bar.h"

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/event_manager.h>

#include <fonts.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#define CHILD_BAR 0
#define CHILD_BATTERY_LABEL 1
#define CHILD_DISCONNECTED_BAR 2
#define CHILD_DISCONNECTED_LABEL 3

#define LOW_BATTERY_THRESHOLD 20

static uint8_t battery_levels[ZMK_SPLIT_BLE_PERIPHERAL_COUNT] = {0};
static bool peripheral_connected[ZMK_SPLIT_BLE_PERIPHERAL_COUNT] = {false};

struct battery_update_state {
    uint8_t source;
    uint8_t level;
};

struct connection_update_state {
    uint8_t source;
    bool connected;
};

static const char *side_name(uint8_t source) {
    return source == 0 ? "L" : "R";
}

static void update_battery_text(lv_obj_t *label, uint8_t source, uint8_t level) {
    lv_label_set_text_fmt(label, "%s %d%%", side_name(source), level);
}

static void update_disconnected_text(lv_obj_t *label, uint8_t source) {
    lv_label_set_text_fmt(label, "%s --", side_name(source));
}

static void set_connected_appearance(lv_obj_t *bar, lv_obj_t *label, uint8_t level) {
    if (level > 0 && level <= LOW_BATTERY_THRESHOLD) {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0xD3900F), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xE8AC11), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x6E4E07), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFB802), LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xF0F0F0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    }
}

static lv_obj_t *peripheral_container(lv_obj_t *widget_obj, uint8_t source) {
    if (source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return NULL;
    }

    return lv_obj_get_child(widget_obj, source);
}

static void refresh_peripheral(lv_obj_t *widget_obj, uint8_t source, bool animate) {
    lv_obj_t *info_container = peripheral_container(widget_obj, source);
    if (!info_container) {
        return;
    }

    lv_obj_t *bar = lv_obj_get_child(info_container, CHILD_BAR);
    lv_obj_t *battery_label = lv_obj_get_child(info_container, CHILD_BATTERY_LABEL);
    lv_obj_t *disconnected_bar = lv_obj_get_child(info_container, CHILD_DISCONNECTED_BAR);
    lv_obj_t *disconnected_label = lv_obj_get_child(info_container, CHILD_DISCONNECTED_LABEL);
    if (!bar || !battery_label || !disconnected_bar || !disconnected_label) {
        return;
    }

    lv_anim_enable_t bar_anim = animate ? LV_ANIM_ON : LV_ANIM_OFF;
    uint8_t level = battery_levels[source];
    bool connected = peripheral_connected[source];

    update_battery_text(battery_label, source, level);
    update_disconnected_text(disconnected_label, source);
    set_connected_appearance(bar, battery_label, level);
    lv_bar_set_value(bar, level, bar_anim);

    lv_anim_del(bar, NULL);
    lv_anim_del(battery_label, NULL);
    lv_anim_del(disconnected_bar, NULL);
    lv_anim_del(disconnected_label, NULL);

    if (connected) {
        lv_obj_fade_out(disconnected_bar, 150, 0);
        lv_obj_fade_out(disconnected_label, 150, 0);
        lv_obj_fade_in(bar, 150, animate ? 150 : 0);
        lv_obj_fade_in(battery_label, 150, animate ? 150 : 0);
    } else {
        lv_obj_fade_out(bar, 150, 0);
        lv_obj_fade_out(battery_label, 150, 0);
        lv_obj_fade_in(disconnected_bar, 150, animate ? 150 : 0);
        lv_obj_fade_in(disconnected_label, 150, animate ? 150 : 0);
    }
}

static void set_battery_bar_value(lv_obj_t *widget_obj, struct battery_update_state state,
                                  bool initialized) {
    if (!initialized || state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return;
    }

    battery_levels[state.source] = state.level;
    refresh_peripheral(widget_obj, state.source, true);
}

static void set_battery_bar_connected(lv_obj_t *widget_obj, struct connection_update_state state,
                                      bool initialized) {
    if (!initialized || state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return;
    }

    peripheral_connected[state.source] = state.connected;
    refresh_peripheral(widget_obj, state.source, true);
}

void battery_bar_battery_update_cb(struct battery_update_state state) {
    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_bar_value(widget->obj, state, widget->initialized);
    }
}

static struct battery_update_state battery_bar_get_battery_state(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct battery_update_state){.source = 0, .level = 0};
    }

    const struct zmk_peripheral_battery_state_changed *bat_ev =
        as_zmk_peripheral_battery_state_changed(eh);
    if (bat_ev == NULL) {
        return (struct battery_update_state){.source = 0, .level = 0};
    }

    return (struct battery_update_state){
        .source = bat_ev->source,
        .level = bat_ev->state_of_charge,
    };
}

void battery_bar_connection_update_cb(struct connection_update_state state) {
    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        set_battery_bar_connected(widget->obj, state, widget->initialized);
    }
}

static struct connection_update_state battery_bar_get_connection_state(const zmk_event_t *eh) {
    if (eh == NULL) {
        return (struct connection_update_state){.source = 0, .connected = false};
    }

    const struct zmk_split_central_status_changed *conn_ev =
        as_zmk_split_central_status_changed(eh);
    if (conn_ev == NULL) {
        return (struct connection_update_state){.source = 0, .connected = false};
    }

    return (struct connection_update_state){
        .source = conn_ev->slot,
        .connected = conn_ev->connected,
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_bar_battery, struct battery_update_state,
                            battery_bar_battery_update_cb, battery_bar_get_battery_state);
ZMK_SUBSCRIPTION(widget_battery_bar_battery, zmk_peripheral_battery_state_changed);

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_bar_connection, struct connection_update_state,
                            battery_bar_connection_update_cb, battery_bar_get_connection_state);
ZMK_SUBSCRIPTION(widget_battery_bar_connection, zmk_split_central_status_changed);

int zmk_widget_battery_bar_init(struct zmk_widget_battery_bar *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_width(widget->obj, lv_pct(100));
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(widget->obj, 16, LV_PART_MAIN);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        lv_obj_t *info_container = lv_obj_create(widget->obj);
        lv_obj_center(info_container);
        lv_obj_set_height(info_container, lv_pct(100));
        lv_obj_set_flex_grow(info_container, 1);

        lv_obj_t *bar = lv_bar_create(info_container);
        lv_obj_set_size(bar, lv_pct(100), 4);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xF0F0F0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 1, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(bar, 250, LV_PART_MAIN);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_opa(bar, 0, LV_PART_MAIN);

        lv_obj_t *battery_label = lv_label_create(info_container);
        lv_obj_set_style_text_font(battery_label, &FG_Medium_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(battery_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_text_align(battery_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_opa(battery_label, 0, LV_PART_MAIN);
        lv_obj_align(battery_label, LV_ALIGN_CENTER, 0, -1);
        update_battery_text(battery_label, i, 0);

        lv_obj_t *disconnected_bar = lv_obj_create(info_container);
        lv_obj_set_size(disconnected_bar, lv_pct(100), 4);
        lv_obj_align(disconnected_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(disconnected_bar, lv_color_hex(0x9E2121), LV_PART_MAIN);
        lv_obj_set_style_radius(disconnected_bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(disconnected_bar, 255, LV_PART_MAIN);

        lv_obj_t *disconnected_label = lv_label_create(info_container);
        lv_obj_set_style_text_font(disconnected_label, &FG_Medium_20, LV_PART_MAIN);
        lv_obj_set_style_text_color(disconnected_label, lv_color_hex(0xE63030), LV_PART_MAIN);
        lv_obj_set_style_text_align(disconnected_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(disconnected_label, LV_ALIGN_CENTER, 0, -1);
        update_disconnected_text(disconnected_label, i);

        refresh_peripheral(widget->obj, i, false);
    }

    sys_slist_append(&widgets, &widget->node);
    widget->initialized = true;
    widget_battery_bar_connection_init();
    widget_battery_bar_battery_init();

    return 0;
}

lv_obj_t *zmk_widget_battery_bar_obj(struct zmk_widget_battery_bar *widget) { return widget->obj; }

static void set_battery_bar_width(void *obj, int32_t width) {
    lv_obj_set_width(obj, width);
}

void zmk_widget_battery_bar_set_compact(bool compact) {
    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, widget->obj);
        lv_anim_set_exec_cb(&a, set_battery_bar_width);
        lv_anim_set_time(&a, 200);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

        int32_t current_width = lv_obj_get_width(widget->obj);
        int32_t target_width = compact ? 227 : 280;

        lv_anim_set_values(&a, current_width, target_width);
        lv_anim_start(&a);
    }
}
