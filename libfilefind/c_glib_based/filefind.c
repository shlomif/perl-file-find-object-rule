/* 
Copyright (C) 2005, 2006 by Olivier Thauvin

This package is free software; you can redistribute it and/or modify it under 
the following terms:

1. The GNU General Public License Version 2.0 - 
http://www.opensource.org/licenses/gpl-license.php

2. The Artistic License Version 2.0 -
http://www.perlfoundation.org/legal/licenses/artistic-2_0.html

3. At your option - any later version of either or both of these licenses.
*/

/* (Modified by Shlomi Fish, while disclaiming all rights. */
/*
 * =====================================================================================
 *
 *       Filename:  filefind.c
 *
 *    Description:  main file of libfilefind.
 *
 *        Created:  01/06/10 21:35:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shlomi Fish via Olivier Thauvin
 *
 * =====================================================================================
 */

#include <glib.h>
#include <glib/gstdio.h>

enum
{
    ACTION_RUN_CB = 0,
    ACTION_SET_OBJ,
    ACTION_RECURSE,
};

#ifdef G_OS_WIN32
typedef struct _g_stat_struct mystat_t;
#else
typedef struct stat mystat_t;
#endif

struct path_component_struct
{
    int actions[2];
    gchar * curr_file;
    GPtrArray * files;
    gchar * last_dir_scanned;
    gboolean open_dir_ret;
    mystat_t stat_ret;
    GPtrArray * traverse_to;
    GTree * inodes;
};

typedef struct path_component_struct path_component_t;

static GPtrArray * string_array_copy(GPtrArray * arr)
{
    GPtrArray * ret;
    int i;
    gchar * string_copy;

    ret = g_ptr_array_sized_new(arr->len);
    if (! ret)
    {
        return NULL;
    }

    for( i = 0 ; i < arr->len ; i++)
    {
        string_copy = g_strdup((gchar *)g_ptr_array_index(arr, i));
        if (! string_copy)
        {
            int prev_i;
            for (prev_i = 0 ; prev_i < i ; prev_i++)
            {
                g_free(g_ptr_array_index(arr, prev_i));
            }
            g_ptr_array_free(arr, FALSE);

            return NULL;
        }
        g_ptr_array_add(ret, string_copy);
    }

    return ret;
}

static GPtrArray * path_component_files_copy(path_component_t * self)
{
    return string_array_copy(self->files);
}

static GPtrArray * path_component_traverse_to_copy(path_component_t * self)
{
    return string_array_copy(self->traverse_to);
}
