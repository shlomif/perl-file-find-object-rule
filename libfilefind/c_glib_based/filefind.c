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
    FILEFIND_STATUS_DONT_SCAN,
    FILEFIND_STATUS_END,
};

struct file_finder_struct;

struct path_component_struct
{
    gint actions[2];
    gchar * curr_file;
    GPtrArray * files;
    gchar * last_dir_scanned;
    gboolean open_dir_ret;
    mystat_t stat_ret;
    GPtrArray * traverse_to;
    gint next_traverse_to_idx;
    GTree * inodes;
    status_t (*move_next)(
        struct path_component_struct * self, 
        struct file_finder_struct * top
    );
};

typedef struct path_component_struct path_component_t;

typedef struct 
{
} item_result_t;

struct file_finder_struct
{
    mystat_t top_stat;
    GPtrArray * dir_stack;
    /* TODO : make curr_comps with GDestroyNotify of g_free. */
    GPtrArray * curr_comps;
    dev_t dev;
    path_component_t * current;
    gchar * curr_path;
    /* The default actions. */
    gint def_actions[2];
    item_result_t * item_obj;
    int target_index;
    GPtrArray * targets;
    gboolean top_is_dir;
    gboolean top_is_link;
    void (*callback)(const char * filename, void * context);
    void * callback_context;
    /* This is ->depth() from File-Find-Object */
    gboolean should_traverse_depth_first;
    int (*filter_callback)(const char * filename, void * context);
    void * filter_context;
    /* This is 'followlink' from File-Find-Object. */
    gboolean should_follow_link;

    /* This is ->nocrossfs() from File-Find-Object. */
    gboolean should_not_cross_fs; 
};

typedef struct file_finder_struct file_finder_t;

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
    self->next_traverse_to_idx = 0;
    
    self->open_dir_ret = TRUE;

    return FILEFIND_STATUS_OK;
}

static status_t path_component_component_open_dir(
        path_component_t * self,
        gchar * dir_str)
{
    if (! path_component_should_scan_dir(self, dir_str))
    {
        return self->open_dir_ret
            ? FILEFIND_STATUS_OK
            : FILEFIND_STATUS_DONT_SCAN
            ;
    }
    else
    {
        return path_component_set_up_dir(self, dir_str);
    }
}

static const gchar * path_component_next_traverse_to(path_component_t * self)
{
    const gchar * next_fn;

    g_assert( self->traverse_to );

    if (self->next_traverse_to_idx == self->traverse_to->len)
    {
        return NULL;
    }

    next_fn = (const gchar *)g_ptr_array_index(
        self->traverse_to,
        self->next_traverse_to_idx
        );
    self->next_traverse_to_idx++;

    return next_fn;
}

typedef struct
{
    dev_t st_dev;
    ino_t st_ino;
} inode_data_t;

static gboolean dup_inode_tree(
        gpointer key,
        gpointer value,
        gpointer data
        )
{
    GTree * new_tree = (GTree *)data;

    /* TODO : add error control in case the allocations failed. */
    g_tree_insert(new_tree, g_memdup(key, sizeof(inode_data_t)), g_memdup(value, sizeof(gint)));

    return FALSE;
}


gint inode_tree_cmp(gconstpointer a_void, gconstpointer b_void)
{
    inode_data_t * a, * b;

    a = (inode_data_t *)a_void;
    b = (inode_data_t *)b_void;

    if (a->st_dev < b->st_dev)
    {
        return -1;
    }
    else if (a->st_dev > b->st_dev)
    {
        return 1;
    }
    else
    {
        if (a->st_ino < b->st_ino)
        {
            return -1;
        }
        else if (a->st_ino > b->st_ino)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

static gint inode_tree_cmp_with_context(gconstpointer a_void, gconstpointer b_void, gpointer user_data)
{
    return inode_tree_cmp(a_void, b_void);
}

void inode_tree_destroy_key(gpointer data)
{
    g_free(data);
}

void inode_tree_destroy_val(gpointer data)
{
    g_free(data);
}


static status_t file_finder_fill_actions(
    file_finder_t * top,
    path_component_t * from
);

static status_t file_finder_open_dir(file_finder_t * top);
static gboolean file_finder_increment_target_index(file_finder_t * top);
static status_t file_finder_calc_curr_path(file_finder_t * top);
static gchar * file_finder_calc_next_target(file_finder_t * top);
static status_t file_finder_mystat(file_finder_t * top);
static path_component_t * file_finder_current_father(file_finder_t * top);

static status_t deep_path_move_next(
    path_component_t * self, 
    file_finder_t * top)
{
    path_component_t * current_father;
    const gchar * next_fn;

    current_father = file_finder_current_father(top);

    next_fn = path_component_next_traverse_to(current_father);

    if (! next_fn)
    {
        return FILEFIND_STATUS_END;
    }

    if (self->curr_file)
    {
        g_free(self->curr_file);
        self->curr_file = NULL;
    }

    if (!(self->curr_file = g_strdup(next_fn)))
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    {
        gchar * prev, * new_elem;
        
        prev = g_ptr_array_index(top->curr_comps, top->curr_comps->len-1);

        if (prev)
        {
            g_free(prev);
            g_ptr_array_index(top->curr_comps, top->curr_comps->len-1) = NULL;
        }

        new_elem = g_strdup(self->curr_file);

        if (! new_elem)
        {
            return FILEFIND_STATUS_OUT_OF_MEM;
        }
        g_ptr_array_index(top->curr_comps, top->curr_comps->len-1) = new_elem; 
    }

    file_finder_calc_curr_path(top);

    file_finder_fill_actions(top, self);

    file_finder_mystat(top);

    return FILEFIND_STATUS_OK;
}

static status_t path_component_move_to_next_target(
    path_component_t * self,
    file_finder_t * top,
    const gchar * * next_target
    )
{
    status_t status;
    gchar * target, * target_copy;

    *next_target = NULL;

    target = file_finder_calc_next_target(top);

    if (target == NULL)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    if (self->curr_file)
    {
        g_free(self->curr_file);
        self->curr_file = NULL;
    }

    self->curr_file = target;

    g_ptr_array_set_size(top->curr_comps, 0);

    target_copy = g_strdup(target);

    if (! target_copy)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    g_ptr_array_add (top->curr_comps, target_copy);

    status = file_finder_calc_curr_path(top);

    if (status == FILEFIND_STATUS_OUT_OF_MEM)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    *next_target = target;

    return FILEFIND_STATUS_OK;
}

static status_t path_component_insert_inode_into_tree(
    path_component_t * self,
    GTree * find, gint depth)
{
    ino_t inode;

    if ((inode = path_component_get_inode(self)))
    {
        inode_data_t data;

        data.st_ino = inode;
        data.st_dev = path_component_get_dev(self);

        g_tree_insert(
            find,
            g_memdup(&data, sizeof(data)), 
            g_memdup(&depth, sizeof(depth))
        );
    }

    return FILEFIND_STATUS_OK;
}

static path_component_t * deep_path_new(
    file_finder_t * top,
    path_component_t * from
)
{
    path_component_t * self;
    GTree * find;
    status_t status;

    self = g_new0(path_component_t, 1);

    if (! self)
    {
        return NULL;
    }

    self->move_next = deep_path_move_next;

    self->stat_ret = top->top_stat;

    find = g_tree_new_full(
        inode_tree_cmp_with_context,
        NULL,
        inode_tree_destroy_key,
        inode_tree_destroy_val
    );

    g_tree_foreach(from->inodes, dup_inode_tree, (gpointer)find);

    path_component_insert_inode_into_tree(self, find, top->dir_stack->len);

    self->inodes = find;

    self->last_dir_scanned = NULL;

    file_finder_fill_actions(top, self);

    g_ptr_array_add(top->curr_comps, g_strdup(""));

    status = file_finder_open_dir(top);

    if (! status)
    {
        return self;
    }
    else
    {
        g_tree_destroy(find);
        g_free(self);

        return NULL;
    }
}

static status_t top_path_move_next(
    path_component_t * self, 
    file_finder_t * top)
{
    status_t status;

    while (file_finder_increment_target_index(top))
    {
        const gchar * next_target;

        /* next_target is a copy and should not be freed */
        status = path_component_move_to_next_target(self, top, &next_target);

        if (status == FILEFIND_STATUS_OUT_OF_MEM)
        {
            return FILEFIND_STATUS_OUT_OF_MEM;
        }

        if (g_file_test(next_target, G_FILE_TEST_EXISTS))
        {
            GTree * find;

            if (file_finder_fill_actions(top, self)
                    == FILEFIND_STATUS_OUT_OF_MEM)
            {
                return FILEFIND_STATUS_OUT_OF_MEM;
            }

            if (file_finder_mystat(top)
                    == FILEFIND_STATUS_OUT_OF_MEM)
            {
                return FILEFIND_STATUS_OUT_OF_MEM;
            }

            self->stat_ret = top->top_stat;
            top->dev = top->top_stat.st_dev;

            find = g_tree_new_full(
                inode_tree_cmp_with_context,
                NULL,
                inode_tree_destroy_key,
                inode_tree_destroy_val
            );

            if (! find)
            {
                return FILEFIND_STATUS_OUT_OF_MEM;
            }

            path_component_insert_inode_into_tree(self, find, 0);

            self->inodes = find;

            return FILEFIND_STATUS_OK;
        }
    }

    return FILEFIND_STATUS_END;
}

static path_component_t * top_path_new(
    file_finder_t * top
)
{
    path_component_t * self;
    status_t status;

    self = g_new0(path_component_t, 1);

    if (! self)
    {
        return NULL;
    }

    self->move_next = top_path_move_next;

    status = file_finder_fill_actions(top, self);

    if (status == FILEFIND_STATUS_OUT_OF_MEM)
    {
        g_free(self);
        return NULL;
    }

    return self;
}

typedef struct 
{
    int stub;
} file_find_handle_t;

enum FILE_FIND_IFACE_STATUS
{
    FILE_FIND_OK = 0,
    FILE_FIND_OUT_OF_MEMORY,
};

static void destroy_string(gpointer data)
{
    g_free((gchar *)data);

    return;
}

int file_find_new(file_find_handle_t * * output_handle, const char * first_target)
{
    file_finder_t * self = NULL;
    path_component_t * top_path = NULL;

    *output_handle = NULL;

    if (! (self = g_new0(file_finder_t, 1)))
    {
        goto cleanup;
    }

    /*  
     * The *existence* of an _st key inside the struct
     * indicates that the stack is full.
     * So now it's empty.
     */

    self->dir_stack = NULL;

    if (! (self->dir_stack = g_ptr_array_new()))
    {
        goto cleanup;
    }

    if (! (self->curr_comps = g_ptr_array_new_with_free_func(destroy_string)))
    {
        goto cleanup;
    }

    if (! (self->targets = g_ptr_array_new_with_free_func(destroy_string)))
    {
        goto cleanup;
    }

    if (first_target)
    {
        gchar * target_copy;

        target_copy = g_strdup(first_target);

        if (! target_copy)
        {
            goto cleanup;
        }

        g_ptr_array_add(self->targets, target_copy);
    }

    self->target_index = -1;

    {
        top_path = top_path_new(self);

        if (! top_path)
        {
            goto cleanup;
        }

        self->current = top_path;

        g_ptr_array_add(self->dir_stack, top_path);
    }

    *output_handle = (file_find_handle_t *)self;

    return FILE_FIND_OK;

cleanup:
    
    if (top_path)
    {
        path_component_free(top_path);
    }
    if (self)
    {
        if (self->dir_stack)
        {
            g_ptr_array_free(self->dir_stack, 1);
            self->dir_stack = NULL;
        }

        if (self->curr_comps)
        {
            g_ptr_array_free(self->dir_stack, 1);
            self->curr_comps = NULL;
        }

        if (self->targets)
        {
            g_ptr_array_free(self->targets, 1);
            self->targets = NULL;
        }
        
        g_free(self);   
    }

    return FILE_FIND_OUT_OF_MEMORY;
}

