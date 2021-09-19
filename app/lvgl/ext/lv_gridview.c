/**
 * @file lv_gridview.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_gridview.h"
#include "core/lv_indev.h"
#include "extra/layouts/grid/lv_grid.h"

#if LV_USE_GRIDVIEW

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_gridview_class

/**********************
 *      TYPEDEFS
 **********************/

typedef struct view_pool_ll_t {
    int id, position;
    lv_obj_t *item;
    struct view_pool_ll_t *prev, *next;
} view_pool_ll_t;

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
    lv_coord_t content_height;
    lv_coord_t pad_row, pad_top, pad_bottom;

    view_pool_ll_t *pool_inuse, *pool_free;
    lv_style_t style_scrollbar, style_scrollbar_scrolled;
} lv_grid_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void update_col_dsc(lv_grid_t *adapter);

static void update_row_dsc(lv_grid_t *adapter, int row_count);

static void update_row_count(lv_grid_t *adapter, int row_count);

static void scroll_cb(lv_event_t *event);

static void fill_rows(lv_grid_t *grid, int row_start, int row_end);

static void update_grid(lv_grid_t *grid, bool rebind);

static void adapter_recycle_item(lv_grid_t *adapter, int position);

static lv_obj_t *adapter_obtain_item(lv_grid_t *grid, int position);

static lv_obj_t *view_pool_take_by_position(view_pool_ll_t **pool, int position);

static bool view_pool_remove_by_instance(view_pool_ll_t **pool, const lv_obj_t *obj);

static view_pool_ll_t *view_pool_node_by_id(view_pool_ll_t *pool, int id);

static view_pool_ll_t *view_pool_node_by_position(view_pool_ll_t *pool, int position);

static view_pool_ll_t *view_pool_node_by_instance(view_pool_ll_t *pool, const lv_obj_t *obj);

static lv_obj_t *view_pool_poll(view_pool_ll_t **pool);

static void view_pool_put(view_pool_ll_t **pool, int id, int position, lv_obj_t *value);

static void view_pool_reset(view_pool_ll_t **pool);

static void lv_gridview_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_gridview_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj);

static void lv_gridview_event(const lv_obj_class_t *class_p, lv_event_t *e);

static void key_cb(lv_grid_t *grid, lv_event_t *e);

static void press_cb(lv_grid_t *grid, lv_event_t *e);

static void update_sizes(lv_grid_t *grid);

static int adapter_item_id(lv_grid_t *grid, int position);

static view_pool_ll_t *view_pool_remove_item(view_pool_ll_t *head, view_pool_ll_t *cur);

static void item_delete_cb(lv_event_t *event);

/**********************
 *  STATIC VARIABLES
 **********************/

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

/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_gridview_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void lv_gridview_set_config(lv_obj_t *obj, int col_count, lv_coord_t row_height) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    bool column_count_changed = grid->column_count != col_count;
    if (column_count_changed) {
        // Scrap all views
        fill_rows(grid, 0, -1);
    }
    grid->column_count = col_count;
    grid->row_height = row_height;
    update_col_dsc(grid);
    if (column_count_changed) {
        int row_count = grid->item_count / grid->column_count;
        if (grid->column_count * row_count < grid->item_count) {
            row_count++;
        }
        update_row_count(grid, row_count);
    }
    update_row_dsc(grid, grid->row_count);
    lv_obj_set_grid_dsc_array(obj, grid->col_dsc, grid->row_dsc);
    if (column_count_changed) {
        update_grid(grid, false);
    }
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
        update_row_count(grid, row_count);
    }
    update_grid(grid, true);
}

void lv_gridview_focus(lv_obj_t *obj, int position) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    if (position < -1 || position >= grid->item_count) return;
    if (grid->focused_index >= 0 && position != grid->focused_index) {
        view_pool_ll_t *node = view_pool_node_by_position(grid->pool_inuse, grid->focused_index);
        if (node) {
            lv_event_send(node->item, LV_EVENT_DEFOCUSED, lv_indev_get_act());
        }
    }
    view_pool_ll_t *node = view_pool_node_by_position(grid->pool_inuse, position);
    lv_obj_t *focused = node ? node->item : NULL;
    if (!focused && position >= 0) {
        lv_obj_t *item = adapter_obtain_item(grid, position);
        int row_idx = position / grid->column_count, col_idx = position % grid->column_count;
        lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
        grid->adapter.bind_view(&grid->obj, item, grid->data, position);
        focused = item;
    }
    if (!focused) return;
    lv_event_send(focused, LV_EVENT_FOCUSED, lv_indev_get_act());
    grid->focused_index = position;
}

int lv_gridview_get_focused_index(lv_obj_t *obj) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    return grid->focused_index;
}

void lv_grid_rebind(lv_obj_t *obj) {
    lv_grid_t *grid = (lv_grid_t *) obj;
    for (int row_idx = LV_MAX(0, grid->row_start); row_idx <= grid->row_end; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int position = row_idx * grid->column_count + col_idx;
            if (position >= grid->item_count) continue;
            view_pool_ll_t *node = view_pool_node_by_position(grid->pool_inuse, position);
            if (!node)return;
            grid->adapter.bind_view(&grid->obj, node->item, grid->data, position);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

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
        case LV_EVENT_SIZE_CHANGED:
            update_sizes(grid);
            update_grid(grid, false);
            break;
        case LV_EVENT_STYLE_CHANGED:
            update_sizes(grid);
            break;
        default:
            return;
    }
}

static void update_col_dsc(lv_grid_t *adapter) {
    int column_count = adapter->column_count;
    lv_coord_t *col_dsc = lv_mem_realloc(adapter->col_dsc, (column_count + 1) * sizeof(lv_coord_t));
    for (int i = 0; i < column_count; i++) {
        col_dsc[i] = LV_GRID_FR(1);
    }
    col_dsc[column_count] = LV_GRID_TEMPLATE_LAST;
    adapter->col_dsc = col_dsc;
}

static void update_row_dsc(lv_grid_t *adapter, int row_count) {
    if (!row_count) {
        row_count = 1;
    }
    lv_coord_t *row_dsc = lv_mem_realloc(adapter->row_dsc, (row_count + 1) * sizeof(lv_coord_t));
    for (int i = 0; i < row_count; i++) {
        row_dsc[i] = adapter->row_height;
    }
    row_dsc[row_count] = LV_GRID_TEMPLATE_LAST;
    adapter->row_dsc = row_dsc;
}

static void update_row_count(lv_grid_t *adapter, int row_count) {
    if (row_count <= 0) return;
    lv_obj_set_grid_cell(adapter->placeholder, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, row_count - 1, 1);
    adapter->row_count = row_count;
}

static void scroll_cb(lv_event_t *event) {
    update_grid((lv_grid_t *) lv_event_get_current_target(event), false);
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
        case LV_KEY_ESC:
            lv_gridview_focus((lv_obj_t *) grid, -1);
            return;
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
    view_pool_ll_t *node = view_pool_node_by_position(grid->pool_inuse, index);
    if (!node) return;
    lv_event_send(node->item, e->code, indev);
}


static void update_sizes(lv_grid_t *grid) {
    lv_obj_t *obj = &grid->obj;
    grid->content_height = lv_obj_get_content_height(obj);
    grid->pad_row = lv_obj_get_style_pad_row(obj, 0);
    grid->pad_top = lv_obj_get_style_pad_top(obj, 0);
    grid->pad_bottom = lv_obj_get_style_pad_bottom(obj, 0);
}

static void update_grid(lv_grid_t *grid, bool rebind) {
    lv_obj_t *obj = &grid->obj;
    int content_height = grid->content_height;
    if (content_height <= 0) return;
    lv_coord_t scroll_y = lv_obj_get_scroll_y(obj);
    lv_coord_t pad_row = grid->pad_row;
    lv_coord_t pad_top = grid->pad_top;
    lv_coord_t pad_bottom = grid->pad_bottom;
    lv_coord_t extend = grid->row_height / 4;
    lv_coord_t row_height = grid->row_height;
    int row_start = -1;
    for (int i = 0; i < grid->row_count; i++) {
        lv_coord_t row_top = row_height * i + pad_row * i;
        lv_coord_t row_bottom = row_top + row_height;
        lv_coord_t start_bound = scroll_y - pad_top - extend;
        if (start_bound <= row_bottom) {
            row_start = i;
            break;
        }
    }
    int row_end = -1;
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
    } else if (rebind) {
        lv_grid_rebind(obj);
    }
}

static void fill_rows(lv_grid_t *grid, int row_start, int row_end) {
    // Mark unused ranges (top + bottom)
    int old_start = grid->row_start, old_end = grid->row_end;
    // Put excess items to recycler
    for (int row_idx = LV_MAX(0, old_start); row_idx < row_start; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int position = row_idx * grid->column_count + col_idx;
            if (position >= grid->item_count) continue;
            adapter_recycle_item(grid, position);
        }
    }
    for (int row_idx = LV_MAX(0, row_end + 1); row_idx <= old_end; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int position = row_idx * grid->column_count + col_idx;
            if (position >= grid->item_count) continue;
            adapter_recycle_item(grid, position);
        }
    }
    // Get needed items
    for (int row_idx = LV_MAX(0, row_start); row_idx < old_start; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int position = row_idx * grid->column_count + col_idx;
            if (position >= grid->item_count) continue;
            lv_obj_t *item = adapter_obtain_item(grid, position);
            lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
            grid->adapter.bind_view(&grid->obj, item, grid->data, position);
        }
    }
    for (int row_idx = LV_MAX(0, old_end + 1); row_idx <= row_end; row_idx++) {
        for (int col_idx = 0; col_idx < grid->column_count; col_idx++) {
            int position = row_idx * grid->column_count + col_idx;
            if (position >= grid->item_count) continue;
            lv_obj_t *item = adapter_obtain_item(grid, position);
            lv_obj_set_grid_cell(item, LV_GRID_ALIGN_STRETCH, col_idx, 1, LV_GRID_ALIGN_STRETCH, row_idx, 1);
            grid->adapter.bind_view(&grid->obj, item, grid->data, position);
        }
    }
    grid->row_start = row_start;
    grid->row_end = row_end;
}

static int adapter_item_id(lv_grid_t *grid, int position) {
    if (!grid->adapter.item_id) return position;
    int id = grid->adapter.item_id(&grid->obj, grid->data, position);
    LV_ASSERT(id >= 0);
    return id;
}

static void adapter_recycle_item(lv_grid_t *adapter, int position) {
    // Move item from inuse pool to free pool
    lv_obj_t *item = view_pool_take_by_position(&adapter->pool_inuse, position);
    LV_ASSERT_MSG(item, "should never recycle invalid item");
    lv_obj_add_flag(item, LV_OBJ_FLAG_HIDDEN);
    view_pool_put(&adapter->pool_free, -1, -1, item);
}

static lv_obj_t *adapter_obtain_item(lv_grid_t *grid, int position) {
    int id = adapter_item_id(grid, position);
    // Move item from free pool to inuse pool, or create one
    view_pool_ll_t *cur = view_pool_node_by_id(grid->pool_inuse, id);
    lv_obj_t *item = cur ? cur->item : NULL;
    if (item) return item;
    item = view_pool_poll(&grid->pool_free);
    if (!item) {
        item = grid->adapter.create_view((lv_obj_t *) grid);
        lv_obj_add_event_cb(item, item_delete_cb, LV_EVENT_DELETE, grid);
    }
    lv_obj_clear_flag(item, LV_OBJ_FLAG_HIDDEN);
    view_pool_put(&grid->pool_inuse, id, position, item);
    return item;
}

static lv_obj_t *view_pool_take_by_position(view_pool_ll_t **pool, int position) {
    view_pool_ll_t *node = view_pool_node_by_position(*pool, position);
    if (!node) return NULL;
    lv_obj_t *item = node->item;
    (*pool) = view_pool_remove_item(*pool, node);
    lv_mem_free(node);
    return item;
}

static bool view_pool_remove_by_instance(view_pool_ll_t **pool, const lv_obj_t *obj) {
    view_pool_ll_t *node = view_pool_node_by_instance(*pool, obj);
    if (!node) return false;
    (*pool) = view_pool_remove_item(*pool, node);
    lv_mem_free(node);
    return true;
}

static view_pool_ll_t *view_pool_node_by_id(view_pool_ll_t *pool, int id) {
    for (view_pool_ll_t *cur = pool; cur != NULL; cur = cur->next) {
        if (cur->id == id) {
            return cur;
        }
    }
    return NULL;
}

static view_pool_ll_t *view_pool_node_by_position(view_pool_ll_t *pool, int position) {
    for (view_pool_ll_t *cur = pool; cur != NULL; cur = cur->next) {
        if (cur->position == position) {
            return cur;
        }
    }
    return NULL;
}

static view_pool_ll_t *view_pool_node_by_instance(view_pool_ll_t *pool, const lv_obj_t *obj) {
    for (view_pool_ll_t *cur = pool; cur != NULL; cur = cur->next) {
        if (cur->item == obj) {
            return cur;
        }
    }
    return NULL;
}

static lv_obj_t *view_pool_poll(view_pool_ll_t **pool) {
    view_pool_ll_t *head = *pool;
    if (!head) return NULL;
    lv_obj_t *item = head->item;
    if (head->next) {
        head->next->prev = NULL;
    }
    *pool = head->next;
    lv_mem_free(head);
    return item;
}

static void view_pool_put(view_pool_ll_t **pool, int id, int position, lv_obj_t *value) {
    view_pool_ll_t *head = lv_mem_alloc(sizeof(view_pool_ll_t));
    lv_memset_00(head, sizeof(view_pool_ll_t));
    head->item = value;
    head->id = id;
    head->position = position;
    view_pool_ll_t *old_head = *pool;
    head->next = old_head;
    if (old_head) {
        old_head->prev = head;
    }
    *pool = head;
}

static view_pool_ll_t *view_pool_remove_item(view_pool_ll_t *head, view_pool_ll_t *cur) {
    LV_ASSERT(head);
    LV_ASSERT(cur);
    view_pool_ll_t *prev = cur->prev;
    if (prev) {
        prev->next = cur->next;
    } else {
        head = cur->next;
    }
    if (cur->next) {
        cur->next->prev = prev;
    }
    return head;
}

static void view_pool_reset(view_pool_ll_t **pool) {
    view_pool_ll_t *cur = *pool;
    while (cur) {
        view_pool_ll_t *tmp = cur;
        cur = cur->next;
        lv_mem_free(tmp);
    }
    *pool = NULL;
}

static void item_delete_cb(lv_event_t *event) {
    lv_grid_t *grid = lv_event_get_user_data(event);
    view_pool_remove_by_instance(&grid->pool_inuse, lv_event_get_current_target(event));
    view_pool_remove_by_instance(&grid->pool_free, lv_event_get_current_target(event));
}

#endif /* LV_USE_GRIDVIEW */