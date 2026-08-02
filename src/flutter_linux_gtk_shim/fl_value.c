// SPDX-License-Identifier: MIT
#include "flutter_linux/fl_value.h"

#include <string.h>

#if !GLIB_CHECK_VERSION(2, 68, 0)
static gpointer g_memdup2(gconstpointer mem, gsize byte_size) {
    if (mem == NULL || byte_size == 0) {
        return NULL;
    }

    gpointer copy = g_malloc(byte_size);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, mem, byte_size);
    return copy;
}
#endif

struct _FlValue {
    GObject parent_instance;
    FlValueType type;
    union {
        gboolean bool_value;
        int64_t int_value;
        double float_value;
        gchar *string_value;
        struct {
            size_t length;
            uint8_t *data;
        } uint8_list;
        struct {
            size_t length;
            int32_t *data;
        } int32_list;
        struct {
            size_t length;
            int64_t *data;
        } int64_list;
        struct {
            size_t length;
            double *data;
        } float_list;
        GPtrArray *list;
        struct {
            GPtrArray *keys;
            GPtrArray *values;
        } map;
    } value;
};

#define fl_value_get_type fl_value_get_gtype
G_DEFINE_TYPE(FlValue, fl_value, G_TYPE_OBJECT)
#undef fl_value_get_type

static void fl_value_finalize(GObject *object) {
    FlValue *self = FL_VALUE(object);

    switch (self->type) {
        case FL_VALUE_TYPE_STRING:
            g_free(self->value.string_value);
            break;
        case FL_VALUE_TYPE_UINT8_LIST:
            g_free(self->value.uint8_list.data);
            break;
        case FL_VALUE_TYPE_INT32_LIST:
            g_free(self->value.int32_list.data);
            break;
        case FL_VALUE_TYPE_INT64_LIST:
            g_free(self->value.int64_list.data);
            break;
        case FL_VALUE_TYPE_FLOAT_LIST:
            g_free(self->value.float_list.data);
            break;
        case FL_VALUE_TYPE_LIST:
            if (self->value.list) {
                for (guint i = 0; i < self->value.list->len; i++) {
                    FlValue *item = g_ptr_array_index(self->value.list, i);
                    if (item) {
                        g_object_unref(item);
                    }
                }
                g_ptr_array_free(self->value.list, TRUE);
            }
            break;
        case FL_VALUE_TYPE_MAP:
            if (self->value.map.keys) {
                for (guint i = 0; i < self->value.map.keys->len; i++) {
                    FlValue *key = g_ptr_array_index(self->value.map.keys, i);
                    FlValue *val = g_ptr_array_index(self->value.map.values, i);
                    if (key) {
                        g_object_unref(key);
                    }
                    if (val) {
                        g_object_unref(val);
                    }
                }
                g_ptr_array_free(self->value.map.keys, TRUE);
                g_ptr_array_free(self->value.map.values, TRUE);
            }
            break;
        default:
            break;
    }

    G_OBJECT_CLASS(fl_value_parent_class)->finalize(object);
}

static void fl_value_class_init(FlValueClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = fl_value_finalize;
}

static void fl_value_init(FlValue *self) {
    self->type = FL_VALUE_TYPE_NULL;
}

FlValueType fl_value_get_type_id(FlValue *value) {
    g_return_val_if_fail(value != NULL, FL_VALUE_TYPE_NULL);
    return value->type;
}

FlValueType fl_value_get_type(FlValue *value) {
    return fl_value_get_type_id(value);
}

static FlValue *fl_value_new_common(FlValueType type) {
    FlValue *value = g_object_new(FL_TYPE_VALUE, NULL);
    value->type = type;
    return value;
}

FlValue *fl_value_new_null(void) {
    return fl_value_new_common(FL_VALUE_TYPE_NULL);
}

FlValue *fl_value_new_bool(gboolean value) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_BOOL);
    v->value.bool_value = value;
    return v;
}

FlValue *fl_value_new_int(int64_t value) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_INT);
    v->value.int_value = value;
    return v;
}

FlValue *fl_value_new_float(double value) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_FLOAT);
    v->value.float_value = value;
    return v;
}

FlValue *fl_value_new_string(const gchar *value) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_STRING);
    v->value.string_value = g_strdup(value ? value : "");
    return v;
}

FlValue *fl_value_new_uint8_list(const uint8_t *value, size_t length) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_UINT8_LIST);
    v->value.uint8_list.length = length;
    v->value.uint8_list.data = length ? g_memdup2(value, length) : NULL;
    return v;
}

FlValue *fl_value_new_int32_list(const int32_t *value, size_t length) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_INT32_LIST);
    v->value.int32_list.length = length;
    v->value.int32_list.data = length ? g_memdup2(value, sizeof(int32_t) * length) : NULL;
    return v;
}

FlValue *fl_value_new_int64_list(const int64_t *value, size_t length) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_INT64_LIST);
    v->value.int64_list.length = length;
    v->value.int64_list.data = length ? g_memdup2(value, sizeof(int64_t) * length) : NULL;
    return v;
}

FlValue *fl_value_new_float_list(const double *value, size_t length) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_FLOAT_LIST);
    v->value.float_list.length = length;
    v->value.float_list.data = length ? g_memdup2(value, sizeof(double) * length) : NULL;
    return v;
}

FlValue *fl_value_new_list(void) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_LIST);
    v->value.list = g_ptr_array_new_with_free_func(NULL);
    return v;
}

FlValue *fl_value_new_map(void) {
    FlValue *v = fl_value_new_common(FL_VALUE_TYPE_MAP);
    v->value.map.keys = g_ptr_array_new_with_free_func(NULL);
    v->value.map.values = g_ptr_array_new_with_free_func(NULL);
    return v;
}

FlValue *fl_value_ref(FlValue *value) {
    return g_object_ref(value);
}

void fl_value_unref(FlValue *value) {
    g_object_unref(value);
}

const gchar *fl_value_get_string(FlValue *value) {
    g_return_val_if_fail(value != NULL, NULL);
    return value->type == FL_VALUE_TYPE_STRING ? value->value.string_value : NULL;
}

int64_t fl_value_get_int(FlValue *value) {
    g_return_val_if_fail(value != NULL, 0);
    return value->type == FL_VALUE_TYPE_INT ? value->value.int_value : 0;
}

double fl_value_get_float(FlValue *value) {
    g_return_val_if_fail(value != NULL, 0.0);
    return value->type == FL_VALUE_TYPE_FLOAT ? value->value.float_value : 0.0;
}

gboolean fl_value_get_bool(FlValue *value) {
    g_return_val_if_fail(value != NULL, FALSE);
    return value->type == FL_VALUE_TYPE_BOOL ? value->value.bool_value : FALSE;
}

size_t fl_value_get_length(FlValue *value) {
    g_return_val_if_fail(value != NULL, 0);

    switch (value->type) {
        case FL_VALUE_TYPE_UINT8_LIST: return value->value.uint8_list.length;
        case FL_VALUE_TYPE_INT32_LIST: return value->value.int32_list.length;
        case FL_VALUE_TYPE_INT64_LIST: return value->value.int64_list.length;
        case FL_VALUE_TYPE_FLOAT_LIST: return value->value.float_list.length;
        case FL_VALUE_TYPE_LIST: return value->value.list ? value->value.list->len : 0;
        case FL_VALUE_TYPE_MAP: return value->value.map.keys ? value->value.map.keys->len : 0;
        default: return 0;
    }
}

const uint8_t *fl_value_get_uint8_list(FlValue *value, size_t *length) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_UINT8_LIST) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }
    if (length) {
        *length = value->value.uint8_list.length;
    }
    return value->value.uint8_list.data;
}

const int32_t *fl_value_get_int32_list(FlValue *value, size_t *length) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_INT32_LIST) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }
    if (length) {
        *length = value->value.int32_list.length;
    }
    return value->value.int32_list.data;
}

const int64_t *fl_value_get_int64_list(FlValue *value, size_t *length) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_INT64_LIST) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }
    if (length) {
        *length = value->value.int64_list.length;
    }
    return value->value.int64_list.data;
}

const double *fl_value_get_float_list(FlValue *value, size_t *length) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_FLOAT_LIST) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }
    if (length) {
        *length = value->value.float_list.length;
    }
    return value->value.float_list.data;
}

FlValue *fl_value_get_list_value(FlValue *value, size_t index) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_LIST || value->value.list == NULL || index >= value->value.list->len) {
        return NULL;
    }
    return g_ptr_array_index(value->value.list, index);
}

FlValue *fl_value_get_map_key(FlValue *value, size_t index) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_MAP || value->value.map.keys == NULL || index >= value->value.map.keys->len) {
        return NULL;
    }
    return g_ptr_array_index(value->value.map.keys, index);
}

FlValue *fl_value_get_map_value(FlValue *value, size_t index) {
    g_return_val_if_fail(value != NULL, NULL);
    if (value->type != FL_VALUE_TYPE_MAP || value->value.map.values == NULL || index >= value->value.map.values->len) {
        return NULL;
    }
    return g_ptr_array_index(value->value.map.values, index);
}

FlValue *fl_value_lookup_string(FlValue *map, const gchar *key) {
    g_return_val_if_fail(map != NULL, NULL);
    g_return_val_if_fail(key != NULL, NULL);
    if (map->type != FL_VALUE_TYPE_MAP || map->value.map.keys == NULL) {
        return NULL;
    }

    for (guint i = 0; i < map->value.map.keys->len; i++) {
        FlValue *k = g_ptr_array_index(map->value.map.keys, i);
        if (k && fl_value_get_type_id(k) == FL_VALUE_TYPE_STRING) {
            const gchar *kstr = fl_value_get_string(k);
            if (kstr && g_strcmp0(kstr, key) == 0) {
                return fl_value_get_map_value(map, i);
            }
        }
    }

    return NULL;
}

void fl_value_append(FlValue *list, FlValue *value) {
    g_return_if_fail(list != NULL);
    g_return_if_fail(value != NULL);
    if (list->type != FL_VALUE_TYPE_LIST) {
        return;
    }
    g_ptr_array_add(list->value.list, g_object_ref(value));
}

void fl_value_append_take(FlValue *list, FlValue *value) {
    g_return_if_fail(list != NULL);
    g_return_if_fail(value != NULL);
    fl_value_append(list, value);
    fl_value_unref(value);
}

void fl_value_set(FlValue *map, FlValue *key, FlValue *value) {
    g_return_if_fail(map != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);
    if (map->type != FL_VALUE_TYPE_MAP) {
        return;
    }
    g_ptr_array_add(map->value.map.keys, g_object_ref(key));
    g_ptr_array_add(map->value.map.values, g_object_ref(value));
}

void fl_value_set_take(FlValue *map, FlValue *key, FlValue *value) {
    g_return_if_fail(map != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);
    fl_value_set(map, key, value);
    fl_value_unref(key);
    fl_value_unref(value);
}

void fl_value_set_string(FlValue *map, const gchar *key, FlValue *value) {
    g_return_if_fail(map != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);
    FlValue *k = fl_value_new_string(key);
    fl_value_set(map, k, value);
    fl_value_unref(k);
}

void fl_value_set_string_take(FlValue *map, const gchar *key, FlValue *value) {
    g_return_if_fail(map != NULL);
    g_return_if_fail(key != NULL);
    g_return_if_fail(value != NULL);
    fl_value_set_string(map, key, value);
    fl_value_unref(value);
}
