// SPDX-License-Identifier: MIT
#ifndef FL_VALUE_H
#define FL_VALUE_H

#include <glib-object.h>
#include <stddef.h>
#include <stdint.h>

G_BEGIN_DECLS

#define fl_value_get_type fl_value_get_gtype
G_DECLARE_FINAL_TYPE(FlValue, fl_value, FL, VALUE, GObject)
#undef fl_value_get_type

#undef FL_TYPE_VALUE
#define FL_TYPE_VALUE (fl_value_get_gtype())

GType fl_value_get_gtype(void);

typedef enum {
    FL_VALUE_TYPE_NULL,
    FL_VALUE_TYPE_BOOL,
    FL_VALUE_TYPE_INT,
    FL_VALUE_TYPE_FLOAT,
    FL_VALUE_TYPE_STRING,
    FL_VALUE_TYPE_UINT8_LIST,
    FL_VALUE_TYPE_INT32_LIST,
    FL_VALUE_TYPE_INT64_LIST,
    FL_VALUE_TYPE_FLOAT_LIST,
    FL_VALUE_TYPE_LIST,
    FL_VALUE_TYPE_MAP,
} FlValueType;

FlValueType fl_value_get_type(FlValue *value);
FlValueType fl_value_get_type_id(FlValue *value);

FlValue *fl_value_new_null(void);
FlValue *fl_value_new_bool(gboolean value);
FlValue *fl_value_new_int(int64_t value);
FlValue *fl_value_new_float(double value);
FlValue *fl_value_new_string(const gchar *value);
FlValue *fl_value_new_uint8_list(const uint8_t *value, size_t length);
FlValue *fl_value_new_int32_list(const int32_t *value, size_t length);
FlValue *fl_value_new_int64_list(const int64_t *value, size_t length);
FlValue *fl_value_new_float_list(const double *value, size_t length);
FlValue *fl_value_new_list(void);
FlValue *fl_value_new_map(void);

FlValue *fl_value_ref(FlValue *value);
void fl_value_unref(FlValue *value);

// Accessors
const gchar *fl_value_get_string(FlValue *value);
int64_t fl_value_get_int(FlValue *value);
double fl_value_get_float(FlValue *value);
gboolean fl_value_get_bool(FlValue *value);
size_t fl_value_get_length(FlValue *value);

const uint8_t *fl_value_get_uint8_list(FlValue *value, size_t *length);
const int32_t *fl_value_get_int32_list(FlValue *value, size_t *length);
const int64_t *fl_value_get_int64_list(FlValue *value, size_t *length);
const double *fl_value_get_float_list(FlValue *value, size_t *length);

FlValue *fl_value_get_list_value(FlValue *value, size_t index);
FlValue *fl_value_get_map_key(FlValue *value, size_t index);
FlValue *fl_value_get_map_value(FlValue *value, size_t index);

FlValue *fl_value_lookup_string(FlValue *map, const gchar *key);

// Mutation
// The plain forms take a reference on the values passed in; the _take forms
// take ownership of them, so the caller must not unref what it hands over.
void fl_value_append(FlValue *list, FlValue *value);
void fl_value_append_take(FlValue *list, FlValue *value);
void fl_value_set(FlValue *map, FlValue *key, FlValue *value);
void fl_value_set_take(FlValue *map, FlValue *key, FlValue *value);
void fl_value_set_string(FlValue *map, const gchar *key, FlValue *value);
void fl_value_set_string_take(FlValue *map, const gchar *key, FlValue *value);

G_END_DECLS

#endif  // FL_VALUE_H
