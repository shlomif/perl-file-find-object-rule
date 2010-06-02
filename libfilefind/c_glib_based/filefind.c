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
 * =========================================================================
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
 * =========================================================================
 */

#include <glib.h>
#include <glib/gstdio.h>

#include "inline.h"

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

typedef gint status_t;

enum FILEFIND_STATUS
{
    FILEFIND_STATUS_OK = 0,
    FILEFIND_STATUS_OUT_OF_MEM,
};

struct path_component_struct
{
    gint actions[2];
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
    gint i;
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
            gint prev_i;
            for (prev_i = 0 ; prev_i < i ; prev_i++)
            {
                g_free(g_ptr_array_index(arr, prev_i));
            }
            /* We need to pass TRUE because we're actually free-ing all the memory. */
            g_ptr_array_free(arr, TRUE);

            return NULL;
        }
        g_ptr_array_add(ret, string_copy);
    }

    return ret;
}

static void string_array_free(GPtrArray * arr)
{
    gint i;

    for( i = 0 ; i < arr->len ; i++)
    {
        g_free(g_ptr_array_index(arr, i));
    }

    g_ptr_array_free(arr, TRUE);

    return;
}

static GPtrArray * path_component_files_copy(path_component_t * self)
{
    return string_array_copy(self->files);
}

static GPtrArray * path_component_traverse_to_copy(path_component_t * self)
{
    return string_array_copy(self->traverse_to);
}

static GCC_INLINE dev_t path_component_get_dev(path_component_t * self)
{
    return self->stat_ret.st_dev;
}

static GCC_INLINE ino_t path_component_get_inode(path_component_t * self)
{
    return self->stat_ret.st_ino;
}

static gboolean path_component_is_same_inode(path_component_t * self, mystat_t * st)
{
    return
    (
           (path_component_get_dev(self) == st->st_dev)
        && (path_component_get_inode(self) == st->st_ino)
        && (path_component_get_inode(self) != 0)
    );
}

static gboolean path_component_should_scan_dir(
    path_component_t * self,
    gchar * dir_str)
{
    if (! g_strcmp0(self->last_dir_scanned, dir_str))
    {
        return FALSE;
    }
    else
    {
        if (self->last_dir_scanned)
        {
            g_free(self->last_dir_scanned);
        }
        self->last_dir_scanned = g_strdup(dir_str);
        return TRUE;
    }
}

gint indirect_lexic_compare(
    gconstpointer a,
    gconstpointer b)
{
    return g_strcmp0((*(const gchar * *)a), (*(const gchar * *)b));
}

static status_t path_component_calc_dir_files(
    path_component_t * self,
    gchar * dir_str)
{
    GPtrArray * files;
    GDir * handle;
    GError * error;

    files = g_ptr_array_new();

    if (! files)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    handle = g_dir_open(dir_str, 0, &error);

    if (! handle)
    {
        /* Handle this error gracefully. */
        self->files = files;
        return FILEFIND_STATUS_OK;
    }
    else
    {
        const gchar * filename;
        gchar * fn_copy;
        while ((filename = g_dir_read_name(handle)))
        {
            fn_copy = g_strdup(filename);
            if (!fn_copy)
            {
                int prev_i;
                for( prev_i = 0 ; prev_i < files->len ; prev_i++)
                {
                    g_free(g_ptr_array_index(files, prev_i));
                }
                g_ptr_array_free(files, TRUE);
                return FILEFIND_STATUS_OUT_OF_MEM;
            }
            g_ptr_array_add(files, fn_copy);
        }
        
        g_dir_close(handle);
        
        g_ptr_array_sort(files, indirect_lexic_compare);

        self->files = files;

        return FILEFIND_STATUS_OK;
    }
}

static status_t path_component_set_up_dir(
    path_component_t * self,
    gchar * dir_str)
{
    status_t ret;

    if (self->files)
    {
        string_array_free(self->files);
        self->files = NULL;
    }

    ret = path_component_calc_dir_files(self, dir_str);

    if (ret)
    {
        return ret;
    }

    if (self->traverse_to)
    {
        string_array_free (self->traverse_to);
        self->traverse_to = NULL;
    }

    if (! (self->traverse_to = path_component_files_copy(self)))
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }
    
    self->open_dir_ret = TRUE;

    return FILEFIND_STATUS_OK;
}

