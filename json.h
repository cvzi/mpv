/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MP_JSON_H
#define MP_JSON_H



#include <stddef.h>
#include <stdint.h>



/**
 * Data format for options and properties. The API functions to get/set
 * properties and options support multiple formats, and this enum describes
 * them.
 */
typedef enum mpv_format {
    /**
     * Invalid. Sometimes used for empty values. This is always defined to 0,
     * so a normal 0-init of mpv_format (or e.g. mpv_node) is guaranteed to set
     * this it to MPV_FORMAT_NONE (which makes some things saner as consequence).
     */
    MPV_FORMAT_NONE             = 0,
    /**
     * The basic type is char*. It returns the raw property string, like
     * using ${=property} in input.conf (see input.rst).
     *
     * NULL isn't an allowed value.
     *
     * Warning: although the encoding is usually UTF-8, this is not always the
     *          case. File tags often store strings in some legacy codepage,
     *          and even filenames don't necessarily have to be in UTF-8 (at
     *          least on Linux). If you pass the strings to code that requires
     *          valid UTF-8, you have to sanitize it in some way.
     *          On Windows, filenames are always UTF-8, and libmpv converts
     *          between UTF-8 and UTF-16 when using win32 API functions. See
     *          the "Encoding of filenames" section for details.
     *
     * Example for reading:
     *
     *     char *result = NULL;
     *     if (mpv_get_property(ctx, "property", MPV_FORMAT_STRING, &result) < 0)
     *         goto error;
     *     printf("%s\n", result);
     *     mpv_free(result);
     *
     * Or just use mpv_get_property_string().
     *
     * Example for writing:
     *
     *     char *value = "the new value";
     *     // yep, you pass the address to the variable
     *     // (needed for symmetry with other types and mpv_get_property)
     *     mpv_set_property(ctx, "property", MPV_FORMAT_STRING, &value);
     *
     * Or just use mpv_set_property_string().
     *
     */
    MPV_FORMAT_STRING           = 1,
    /**
     * The basic type is char*. It returns the OSD property string, like
     * using ${property} in input.conf (see input.rst). In many cases, this
     * is the same as the raw string, but in other cases it's formatted for
     * display on OSD. It's intended to be human readable. Do not attempt to
     * parse these strings.
     *
     * Only valid when doing read access. The rest works like MPV_FORMAT_STRING.
     */
    MPV_FORMAT_OSD_STRING       = 2,
    /**
     * The basic type is int. The only allowed values are 0 ("no")
     * and 1 ("yes").
     *
     * Example for reading:
     *
     *     int result;
     *     if (mpv_get_property(ctx, "property", MPV_FORMAT_FLAG, &result) < 0)
     *         goto error;
     *     printf("%s\n", result ? "true" : "false");
     *
     * Example for writing:
     *
     *     int flag = 1;
     *     mpv_set_property(ctx, "property", MPV_FORMAT_FLAG, &flag);
     */
    MPV_FORMAT_FLAG             = 3,
    /**
     * The basic type is int64_t.
     */
    MPV_FORMAT_INT64            = 4,
    /**
     * The basic type is double.
     */
    MPV_FORMAT_DOUBLE           = 5,
    /**
     * The type is mpv_node.
     *
     * For reading, you usually would pass a pointer to a stack-allocated
     * mpv_node value to mpv, and when you're done you call
     * mpv_free_node_contents(&node).
     * You're expected not to write to the data - if you have to, copy it
     * first (which you have to do manually).
     *
     * For writing, you construct your own mpv_node, and pass a pointer to the
     * API. The API will never write to your data (and copy it if needed), so
     * you're free to use any form of allocation or memory management you like.
     *
     * Warning: when reading, always check the mpv_node.format member. For
     *          example, properties might change their type in future versions
     *          of mpv, or sometimes even during runtime.
     *
     * Example for reading:
     *
     *     mpv_node result;
     *     if (mpv_get_property(ctx, "property", MPV_FORMAT_NODE, &result) < 0)
     *         goto error;
     *     printf("format=%d\n", (int)result.format);
     *     mpv_free_node_contents(&result).
     *
     * Example for writing:
     *
     *     mpv_node value;
     *     value.format = MPV_FORMAT_STRING;
     *     value.u.string = "hello";
     *     mpv_set_property(ctx, "property", MPV_FORMAT_NODE, &value);
     */
    MPV_FORMAT_NODE             = 6,
    /**
     * Used with mpv_node only. Can usually not be used directly.
     */
    MPV_FORMAT_NODE_ARRAY       = 7,
    /**
     * See MPV_FORMAT_NODE_ARRAY.
     */
    MPV_FORMAT_NODE_MAP         = 8,
    /**
     * A raw, untyped byte array. Only used only with mpv_node, and only in
     * some very specific situations. (Some commands use it.)
     */
    MPV_FORMAT_BYTE_ARRAY       = 9
} mpv_format;




/**
 * Generic data storage.
 *
 * If mpv writes this struct (e.g. via mpv_get_property()), you must not change
 * the data. In some cases (mpv_get_property()), you have to free it with
 * mpv_free_node_contents(). If you fill this struct yourself, you're also
 * responsible for freeing it, and you must not call mpv_free_node_contents().
 */
typedef struct mpv_node {
    union {
        char *string;   /** valid if format==MPV_FORMAT_STRING */
        int flag;       /** valid if format==MPV_FORMAT_FLAG   */
        int64_t int64;  /** valid if format==MPV_FORMAT_INT64  */
        double double_; /** valid if format==MPV_FORMAT_DOUBLE */
        /**
         * valid if format==MPV_FORMAT_NODE_ARRAY
         *    or if format==MPV_FORMAT_NODE_MAP
         */
        struct mpv_node_list *list;
        /**
         * valid if format==MPV_FORMAT_BYTE_ARRAY
         */
        struct mpv_byte_array *ba;
    } u;
    /**
     * Type of the data stored in this struct. This value rules what members in
     * the given union can be accessed. The following formats are currently
     * defined to be allowed in mpv_node:
     *
     *  MPV_FORMAT_STRING       (u.string)
     *  MPV_FORMAT_FLAG         (u.flag)
     *  MPV_FORMAT_INT64        (u.int64)
     *  MPV_FORMAT_DOUBLE       (u.double_)
     *  MPV_FORMAT_NODE_ARRAY   (u.list)
     *  MPV_FORMAT_NODE_MAP     (u.list)
     *  MPV_FORMAT_BYTE_ARRAY   (u.ba)
     *  MPV_FORMAT_NONE         (no member)
     *
     * If you encounter a value you don't know, you must not make any
     * assumptions about the contents of union u.
     */
    mpv_format format;
} mpv_node;

/**
 * (see mpv_node)
 */
typedef struct mpv_node_list {
    /**
     * Number of entries. Negative values are not allowed.
     */
    int num;
    /**
     * MPV_FORMAT_NODE_ARRAY:
     *  values[N] refers to value of the Nth item
     *
     * MPV_FORMAT_NODE_MAP:
     *  values[N] refers to value of the Nth key/value pair
     *
     * If num > 0, values[0] to values[num-1] (inclusive) are valid.
     * Otherwise, this can be NULL.
     */
    mpv_node *values;
    /**
     * MPV_FORMAT_NODE_ARRAY:
     *  unused (typically NULL), access is not allowed
     *
     * MPV_FORMAT_NODE_MAP:
     *  keys[N] refers to key of the Nth key/value pair. If num > 0, keys[0] to
     *  keys[num-1] (inclusive) are valid. Otherwise, this can be NULL.
     *  The keys are in random order. The only guarantee is that keys[N] belongs
     *  to the value values[N]. NULL keys are not allowed.
     */
    char **keys;
} mpv_node_list;

/**
 * (see mpv_node)
 */
typedef struct mpv_byte_array {
    /**
     * Pointer to the data. In what format the data is stored is up to whatever
     * uses MPV_FORMAT_BYTE_ARRAY.
     */
    void *data;
    /**
     * Size of the data pointed to by ptr.
     */
    size_t size;
} mpv_byte_array;


















int json_parse(void *ta_parent, struct mpv_node *dst, char **src, int max_depth);
void json_skip_whitespace(char **src);
int json_write(char **s, struct mpv_node *src);
int json_write_pretty(char **s, struct mpv_node *src);

#endif
