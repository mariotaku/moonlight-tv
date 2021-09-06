/**
 * @file lv_obj.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_gridview.h"
#include "util/logging.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_gridview_class

typedef struct view_pool_item_t {
    int key;
    lv_obj_t *item;
} view_pool_item_t;

typedef struct view_pool_t {
    view_pool_item_t *data;
    int capacity;
    int size;
} view_pool_t;

typedef struct lv_grid_t {
    lv_obj_t obj;
    lv_grid_adapter_t adapter;
    /*Configs*/
    int column_count;
    lv_coord_t row_height;

    void *data;
    /*States*/
    int item_count, row_count;
    int row_start, row_end;
    int focused_index;
    /*Tmp data*/
    lv_coord_t *col_dsc, *row_dsc;
    lv_obj_t *placeholder;

    view_pool_t pool_inuse, pool_free;
    lv_style_t style_scrollbar, style_scrollbar_scrolled;
} lv_grid_t;

static void update_col_dsc(lv_grid_t *adapter);

static void update_row_dsc(lv_grid_t *adapter, int row_count);

static void update_placeholder(lv_grid_t *adapter, int row_count);

static void scroll_cb(lv_event_t *event);

static void fill_rows(lv_grid_t *grid, int row_start, int row_end);

static void update_grid(lv_grid_t *grid);

static void adapter_recycle_item(lv_grid_t *adapter, int index);

static lv_obj_t *adapter_obtain_item(lv_grid_t *grid, int index);

static lv_obj_t *view_pool_take(view_pool_t *pool, int key);

static lv_obj_t *view_pool_find(view_pool_t *pool, int key);

static lv_obj_t *view_pool_pop(view_pool_t *pool);

static void view_pool_put(view_pool_t *pool, int key, lv_obj_t *value);

static void view_pool_reset(view_pool_t *pool);

static void lv_gridview_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_gridview_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_gridview_event(const lv_obj_class_t *class_p, lv_event_t *e);

static void key_cb(lv_grid_t *grid, lv_event_t *e);

static void press_cb(lv_grid_t *grid, lv_event_t *e);

const lv_obj_class_t lv_gridview_class = {
        .constructor_cb = lv_gridview_constructor,
        .destructor_cb = lv_gridview_destructor,
        .event_cb = lv_gridview_event,
        .width_def = LV_PCT(100),
        .height_def = LV_PCT(100),
        .editable = LV_OBJ_CLASS_EDITABLE_TRUE,
        .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
        .instance_size = (sizeof(lv_grid_t)),
        .base_class = &lv_obj_class,
};

lv_obj_t *lv_gridview_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_gridview_set_config(lv_obj_t *obj, int col_count, lv_coord_t row_height) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    grid->column_count = col_count;
    grid->row_height = row_height;
    update_col_dsc(grid);
}

void lv_grid_set_adapter(lv_obj_t *obj, const lv_grid_adapter_t *adapter) {
    lv_memcpy(&((lv_grid_t *) obj)->adapter, adapter, sizeof(lv_grid_adapter_t));
}

void lv_grid_set_data(lv_obj_t *obj, void *data) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    if (grid->data == data) return;
    grid->data = data;
    if (grid->row_dsc) {
        lv_mem_free(grid->row_dsc);
        grid->row_dsc = NULL;
    }
    if (data) {
        int item_count = grid->adapter.item_count(obj, data);
        grid->item_count = item_count;
        int row_count = item_count / grid->column_count;
        if (grid->column_count * row_count < item_count) {
            row_count++;
        }
        update_row_dsc(grid, row_count);
        lv_obj_set_grid_dsc_array(obj, grid->col_dsc, grid->row_dsc);
        update_placeholder(grid, row_count);
    }
    update_grid(grid);
}

void lv_gridview_focus(lv_obj_t *obj, int index) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    if (index < -1 || index >= grid->item_count) return;
    if (grid->focused_index >= 0 && index != grid->focused_index) {
        lv_obj_t *defocused = view_pool_find(&grid->pool_inuse, grid->focused_index);
        if (defocused) {
            lv_event_send(defocused, LV_EVENT_DEFOCUSED, lv_indev_get_act());
        }
    }
    lv_obj_t *focused = view_pool_find(&grid->pool_inuse, index);
    if (!focused && index >= 0) {
        lv_obj_t *item = adapter_obtain_item(grid, index);
        int row_idx = index / grid->column_count, col_idx = index % grid->column_count;
        lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
        grid->adapter.bind_view(&grid->obj, item, grid->data, index);
        focused = item;
    }
    if (!focused) return;
    lv_event_send(focused, LV_EVENT_FOCUSED, lv_indev_get_act());
    grid->focused_index = index;
}

int lv_gridview_get_focused_index(lv_obj_t *obj) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    return grid->focused_index;
}

static void lv_gridview_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    lv_obj_set_layout(obj, LV_LAYOUT_GRID);

    lv_gridview_set_config(obj, 1, 1);

    lv_obj_t *placeholder = lv_obj_create(obj);

    // Here we steal scroll bar style for parent
    lv_style_init(&grid->style_scrollbar);
    lv_style_set_bg_color(&grid->style_scrollbar, lv_obj_get_style_bg_color(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_radius(&grid->style_scrollbar, lv_obj_get_style_radius(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_pad_right(&grid->style_scrollbar, lv_obj_get_style_pad_right(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_pad_top(&grid->style_scrollbar, lv_obj_get_style_pad_top(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_size(&grid->style_scrollbar, lv_obj_get_style_width(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_bg_opa(&grid->style_scrollbar, lv_obj_get_style_bg_opa(placeholder, LV_PART_SCROLLBAR));
    lv_style_set_transition(&grid->style_scrollbar, lv_obj_get_style_transition(placeholder, LV_PART_SCROLLBAR));

    lv_style_init(&grid->style_scrollbar_scrolled);
    lv_style_set_bg_opa(&grid->style_scrollbar_scrolled, LV_OPA_COVER);

    lv_obj_add_style(obj, &grid->style_scrollbar, LV_PART_SCROLLBAR);
    lv_obj_add_style(obj, &grid->style_scrollbar_scrolled, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);


    lv_obj_set_style_radius(placeholder, 0, 0);
    lv_obj_set_style_border_side(placeholder, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_bg_opa(placeholder, 0, 0);
    lv_obj_set_style_clip_corner(placeholder, false, 0);
    grid->placeholder = placeholder;
    grid->row_start = -1;
    grid->row_end = -1;
    grid->focused_index = -1;
}

static void lv_gridview_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    view_pool_reset(&grid->pool_free);
    view_pool_reset(&grid->pool_inuse);
    if (grid->row_dsc) {
        lv_mem_free(grid->row_dsc);
    }
    lv_mem_free(grid->col_dsc);
}

static void lv_gridview_event(const lv_obj_class_t *class_p, lv_event_t *e) {
    LV_UNUSED(class_p);

    /*Call the ancestor's event handler*/
    lv_res_t res;

    res = lv_obj_event_base(MY_CLASS, e);
    if (res != LV_RES_OK) return;
    lv_grid_t *grid = (lv_grid_t *) e->current_target;
    if (e->target != e->current_target) return;

    switch (e->code) {
        case LV_EVENT_SCROLL:
            scroll_cb(e);
            break;
        case LV_EVENT_KEY:
            key_cb(grid, e);
            break;
        case LV_EVENT_PRESSED:
        case LV_EVENT_PRESSING:
        case LV_EVENT_RELEASED:
        case LV_EVENT_CLICKED:
        case LV_EVENT_PRESS_LOST:
        case LV_EVENT_SHORT_CLICKED:
        case LV_EVENT_LONG_PRESSED:
        case LV_EVENT_LONG_PRESSED_REPEAT:
            press_cb(grid, e);
            break;
        default:
            return;
    }
}

static void update_col_dsc(lv_grid_t *adapter) {
    int column_count = adapter->column_count;
    lv_coord_t *col_dsc = lv_mem_alloc((column_count + 1) * sizeof(lv_coord_t));
    for (int i = 0; i < column_count; i++) {
        col_dsc[i] = LV_GRID_FR(1);
    }
    col_dsc[column_count] = LV_GRID_TEMPLATE_LAST;
    adapter->col_dsc = col_dsc;
}

static void update_row_dsc(lv_grid_t *adapter, int row_count) {
    lv_coord_t *row_dsc = lv_mem_alloc((row_count + 1) * sizeof(lv_coord_t));
    for (int i = 0; i < row_count; i++) {
        row_dsc[i] = adapter->row_height;
    }
    row_dsc[row_count] = LV_GRID_TEMPLATE_LAST;
    adapter->row_dsc = row_dsc;
}

static void update_placeholder(lv_grid_t *adapter, int row_count) {
    lv_obj_set_grid_cell(adapter->placeholder, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, row_count - 1, 1);
    adapter->row_count = row_count;
}

static void scroll_cb(lv_event_t *event) {
    update_grid((lv_grid_t *) lv_event_get_current_target(event));
}

static void key_cb(lv_grid_t *grid, lv_event_t *e) {
    int offset;
    switch (lv_event_get_key(e)) {
        case LV_KEY_LEFT:
            offset = -1;
            break;
        case LV_KEY_RIGHT:
            offset = 1;
            break;
        case LV_KEY_UP:
            offset = -grid->column_count;
            break;
        case LV_KEY_DOWN:
            offset = grid->column_count;
            break;
        default:
            return;
    }
    int index = grid->focused_index;
    if (index < 0) {
        index = grid->row_start * grid->column_count;
    } else {
        index += offset;
    }
    lv_gridview_focus((lv_obj_t *) grid, LV_CLAMP(0, index, grid->item_count - 1));
}

static void press_cb(lv_grid_t *grid, lv_event_t *e) {
    if (e->current_target != e->target) return;
    lv_indev_t *indev = lv_indev_get_act();
    if (lv_indev_get_type(indev) != LV_INDEV_TYPE_KEYPAD) {
        return;
    }
    int index = grid->focused_index;
    if (index < 0) return;
    lv_obj_t *focused = view_pool_find(&grid->pool_inuse, index);
    if (!focused) return;
    lv_event_send(focused, e->code, indev);
}

static void update_grid(lv_grid_t *grid) {
    lv_obj_t *obj = &grid->obj;
    lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
    lv_coord_t content_height = lv_obj_get_content_height(obj);
    lv_coord_t pad_row = lv_obj_get_style_pad_row(obj, 0);
    lv_coord_t pad_top = lv_obj_get_style_pad_top(obj, 0);
    lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(obj, 0);
    lv_coord_t extend = grid->row_height / 4;
    lv_coord_t row_height = grid->row_height;
    int row_start = 0;
    for (int i = 0; i < grid->row_count; i++) {
        lv_coord_t row_top = row_height * i + pad_row * i;
        lv_coord_t row_bottom = row_top + row_height;
        lv_coord_t start_bound = scroll_y - pad_top - extend;
        if (start_bound <= row_bottom) {
            row_start = i;
            break;
        }
    }
    int row_end = 0;
    for (int i = grid->row_count - 1; i >= row_start; i--) {
        lv_coord_t row_top = row_height * i + pad_row * i;
        lv_coord_t end_bound = scroll_y + content_height + pad_bottom + extend;
        if (end_bound >= row_top) {
            row_end = i;
            break;
        }
    }
    if (grid->row_start != row_start || grid->row_end != row_end) {
        fill_rows(grid, row_start, row_end);
        grid->row_start = row_start;
        grid->row_end = row_end;
    }
}

static void fill_rows(lv_grid_t *grid, int row_start, int row_end) {
    // Mark unused ranges (top + bottom)
    int old_start = grid->row_start, old_end = grid->row_end;
    // Put excess items to recycler
    for (int row_idx = LV_MAX(0, old_start); row_idx < row_start; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int index = row_idx * grid->column_count + col_idx;
            if (index >= grid->item_count) continue;
            adapter_recycle_item(grid, index);
        }
    }
    for (int row_idx = LV_MAX(0, row_end + 1); row_idx <= old_end; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int index = row_idx * grid->column_count + col_idx;
            if (index >= grid->item_count) continue;
            adapter_recycle_item(grid, index);
        }
    }
    // Get needed items
    for (int row_idx = LV_MAX(0, row_start); row_idx < old_start; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int index = row_idx * grid->column_count + col_idx;
            if (index >= grid->item_count) continue;
            lv_obj_t *item = adapter_obtain_item(grid, index);
            lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
            grid->adapter.bind_view(&grid->obj, item, grid->data, index);
        }
    }
    for (int row_idx = LV_MAX(0, old_end + 1); row_idx <= row_end; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int index = row_idx * grid->column_count + col_idx;
            if (index >= grid->item_count) continue;
            lv_obj_t *item = adapter_obtain_item(grid, index);
            lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
            grid->adapter.bind_view(&grid->obj, item, grid->data, index);
        }
    }
}

static void adapter_recycle_item(lv_grid_t *adapter, int index) {
    // Move item from inuse pool to free pool
    lv_obj_t *item = view_pool_take(&adapter->pool_inuse, index);
    assert(item);
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    view_pool_put(&adapter->pool_free, -1, item);
}

static lv_obj_t *adapter_obtain_item(lv_grid_t *grid, int index) {
    // Move item from free pool to inuse pool, or create one
    lv_obj_t *item = view_pool_find(&grid->pool_inuse, index);
    if (item) return item;
    item = view_pool_pop(&grid->pool_free);
    if (!item) {
        item = grid->adapter.create_view((lv_obj_t *) grid);
    }
    lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
    view_pool_put(&grid->pool_inuse, index, item);
    return item;
}

static lv_obj_t *view_pool_take(view_pool_t *pool, int key) {
    for (int i = 0; i < pool->size; i++) {
        view_pool_item_t *data = pool->data;
        if (data[i].key == key) {
            lv_obj_t *result = data[i].item;
            pool->size--;
            if (pool->size - i > 0) {
                memmove(&data[i], &data[i + 1], (pool->size - i) * sizeof(view_pool_item_t));
            }
            return result;
        }
    }
    return NULL;
}

static lv_obj_t *view_pool_find(view_pool_t *pool, int key) {
    for (int i = 0; i < pool->size; i++) {
        view_pool_item_t *data = pool->data;
        if (data[i].key == key) {
            return data[i].item;
        }
    }
    return NULL;
}

static lv_obj_t *view_pool_pop(view_pool_t *pool) {
    if (!pool->size) return NULL;
    pool->size--;
    return pool->data[pool->size].item;
}

static void view_pool_put(view_pool_t *pool, int key, lv_obj_t *value) {
    if (!pool->data) {
        pool->capacity = 16;
        pool->data = lv_mem_alloc(pool->capacity * sizeof(view_pool_item_t));
        assert(pool->data);
    } else if (pool->size + 1 > pool->capacity) {
        pool->capacity *= 2;
        pool->data = lv_mem_realloc(pool->data, pool->capacity * sizeof(view_pool_item_t));
        assert(pool->data);
    }
    int index = pool->size;
    pool->data[index].key = key;
    pool->data[index].item = value;
    pool->size++;
}

static void view_pool_reset(view_pool_t *pool) {
    if (pool->data) {
        lv_mem_free(pool->data);
        pool->data = NULL;
    }
    pool->size = 0;
    pool->capacity = 0;
}