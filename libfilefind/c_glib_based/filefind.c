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
#include <string.h>

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
    FILEFIND_STATUS_FALSE,
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
    gchar * path;
    gchar * basename;
    GPtrArray * dir_components;
    gchar * base;
    mystat_t stat_ret;
    gboolean is_file;
    gboolean is_dir;
    gboolean is_link;
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


static void file_finder_fill_actions(
    file_finder_t * top,
    path_component_t * from
);

static status_t file_finder_open_dir(file_finder_t * top);
static gboolean file_finder_increment_target_index(file_finder_t * top);
static status_t file_finder_calc_curr_path(file_finder_t * top);
static gchar * file_finder_calc_next_target(file_finder_t * top);
static status_t file_finder_mystat(file_finder_t * top);

static GCC_INLINE path_component_t * file_finder_current_father(
    file_finder_t * top
    )
{
    GPtrArray * dir_stack;

    dir_stack = top->dir_stack;

    return g_ptr_array_index(dir_stack, dir_stack->len-2);
}

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

            file_finder_fill_actions(top, self);

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

    self = g_new0(path_component_t, 1);

    if (! self)
    {
        return NULL;
    }

    self->move_next = top_path_move_next;

    file_finder_fill_actions(top, self);

    return self;
}

static void path_component_free(path_component_t * self)
{
    if (self->curr_file)
    {
        g_free(self->curr_file);
        self->curr_file = NULL;
    }

    if (self->files)
    {
        g_ptr_array_free(self->files, TRUE);
        self->files = NULL;
    }

    if (self->last_dir_scanned)
    {
        g_free( self->last_dir_scanned );
        self->last_dir_scanned = NULL;
    }

    if (self->traverse_to)
    {
        g_ptr_array_free(self->traverse_to, TRUE);
        self->traverse_to = NULL;
    }

    if (self->inodes)
    {
        g_tree_destroy(self->inodes);
        self->inodes = NULL;
    }
    
    g_free(self);

    return;
}

typedef struct 
{
    int stub;
} file_find_handle_t;

enum FILE_FIND_IFACE_STATUS
{
    FILE_FIND_OK = 0,
    FILE_FIND_OUT_OF_MEMORY,
    FILE_FIND_END,
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

    self->callback = NULL;
    self->callback_context = NULL;
    self->should_traverse_depth_first = FALSE;
    self->filter_callback = NULL;
    self->filter_context = NULL;
    self->should_follow_link = FALSE;
    self->should_not_cross_fs = FALSE;

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

static GCC_INLINE gboolean file_finder_curr_not_a_dir(file_finder_t * self)
{
    return (!self->top_is_dir);
}

/* 
 * Calculates curr_path from self->curr_comps.
 * Must be called whenever curr_comps is modified.
 */ 
static status_t file_finder_calc_curr_path(file_finder_t * self)
{
    gchar * * components;
    int i;

    if (self->curr_path)
    {
        g_free(self->curr_path);
        self->curr_path = NULL;
    }

    components = g_new0(gchar *, self->curr_comps->len+1);
    if (! components)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    for ( i = 0 ; i < self->curr_comps->len ; i++)
    {
        components[i] = g_ptr_array_index (self->curr_comps, i);
    }
    components[i] = NULL;

    if (! (self->curr_path = g_build_filenamev(components)))
    {
        g_free(components);
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    g_free(components);

    return FILEFIND_STATUS_OK;
}

static void item_result_free(item_result_t * item)
{
    if (item->path)
    {
        g_free(item->path);
        item->path = NULL;
    }

    if (item->base)
    {
        g_free(item->base);
        item->base = NULL;
    }

    if (item->dir_components)
    {
        g_ptr_array_free(item->dir_components, 1);
    }

    g_free(item);

    return;
}

static status_t file_finder_calc_currrent_item_obj(
    file_finder_t * self,
    item_result_t * * item
    )
{
    item_result_t * ret;
    int comp_idx, end_comp_idx;
    gboolean curr_not_a_dir;
    GPtrArray * dir_components;
    gchar * comp_copy;

    *item = NULL;
    dir_components = NULL;

    ret = g_new0(item_result_t, 1);
    
    if (!ret)
    {
        return FILEFIND_STATUS_OUT_OF_MEM;
    }

    ret->path = NULL;
    ret->basename = NULL;
    ret->dir_components = NULL;
    ret->base = NULL;
    
    if (! (ret->path = g_strdup(self->curr_path)))
    {
        goto cleanup;
    }

    comp_idx = 0;

    if (!(ret->base = g_strdup(g_ptr_array_index(self->curr_comps, comp_idx))))
    {
        goto cleanup;
    }

    comp_idx++;

    curr_not_a_dir = file_finder_curr_not_a_dir(self);

    end_comp_idx = self->curr_comps->len - 1 - (curr_not_a_dir ? 1 : 0);

    dir_components = g_ptr_array_new_with_free_func(destroy_string);
       
    for (; comp_idx < end_comp_idx ; comp_idx++)
    {
        comp_copy =
            g_strdup(
                g_ptr_array_index(self->curr_comps, comp_idx)
            );

        if (! comp_copy)
        {
            goto cleanup;
        }
        g_ptr_array_add(dir_components, comp_copy);
    }

    ret->dir_components = dir_components;
    /* Avoid double-free. */
    dir_components = NULL;

    if (curr_not_a_dir)
    {
        if (!(ret->basename = 
            g_strdup(
                g_ptr_array_index(self->curr_comps, comp_idx)
            ))
        )
        {
            goto cleanup;
        }
    }

    ret->stat_ret = self->top_stat;
    ret->is_file = g_file_test(self->curr_path, G_FILE_TEST_IS_REGULAR);
    ret->is_dir = g_file_test(self->curr_path, G_FILE_TEST_IS_DIR);
    ret->is_link = g_file_test(self->curr_path, G_FILE_TEST_IS_SYMLINK);

    *item = ret;

    return FILEFIND_STATUS_OK;

cleanup:

    item_result_free(ret);
    if (dir_components)
    {
        g_ptr_array_free(dir_components, 1);
    }
    
    ret = NULL;
    return FILEFIND_STATUS_OUT_OF_MEM;
}

static status_t file_finder_process_current(file_finder_t * top);
static status_t file_finder_master_move_to_next(file_finder_t * top);
static status_t file_finder_me_die(file_finder_t * top);

int file_find_next(file_find_handle_t * handle)
{
    file_finder_t * self;
    status_t total_status, local_status;

    self = (file_finder_t *)handle;

    total_status = FILEFIND_STATUS_FALSE; 
    while (! (total_status == FILEFIND_STATUS_OK))
    {
        total_status = file_finder_process_current(self);
        if (total_status == FILEFIND_STATUS_OUT_OF_MEM)
        {
            goto cleanup;
        }
        else if (total_status == FILEFIND_STATUS_FALSE)
        {
            local_status = file_finder_master_move_to_next(self);
            
            if (local_status == FILEFIND_STATUS_OUT_OF_MEM)
            {
                goto cleanup;
            }
            else if (local_status == FILEFIND_STATUS_END)
            {
                total_status = FILEFIND_STATUS_OK;
            }
            else /* (local_status == FILEFIND_STATUS_OK) */
            {
                total_status = file_finder_me_die(self);

                if (total_status == FILEFIND_STATUS_OUT_OF_MEM)
                {
                    goto cleanup;
                }
            }
        }
    }

    return (self->item_obj ? FILE_FIND_OK : FILE_FIND_END);

cleanup:
    return FILE_FIND_OUT_OF_MEMORY;
}

const gchar * file_find_get_path(file_find_handle_t * handle)
{
    file_finder_t * self;

    self = (file_finder_t *)handle;

    return self->item_obj ? self->item_obj->path : NULL;
}

static gboolean file_finder_increment_target_index(file_finder_t * self)
{
    return (++self->target_index < self->targets->len);
}

/* 
 * TODO : can this return NULL if it reached the end rather than ran out
 * of memory?
 * */
static gchar * file_finder_calc_next_target(file_finder_t * self)
{
    gchar * target;

    target = g_ptr_array_index(self->targets, self->target_index);

    if (! target)
    {
        return NULL;
    }
    else
    {
        return g_strdup(target);
    }
}

static status_t file_finder_master_move_to_next(file_finder_t * self)
{
    return self->current->move_next(self->current, self);
}

static status_t file_finder_become_default(file_finder_t * self);

static status_t file_finder_me_die(file_finder_t * self)
{
    if (self->dir_stack->len > 1)
    {
        return file_finder_become_default(self);
    }
    else
    {
        self->item_obj = NULL;

        return FILEFIND_STATUS_OK;
    }
}

static status_t file_finder_become_default(file_finder_t * self)
{
    path_component_free( 
        (path_component_t *)g_ptr_array_index(self->dir_stack, self->dir_stack->len - 1) 
    );
    g_ptr_array_remove_index (self->dir_stack, self->dir_stack->len - 1);

    self->current =
        (path_component_t *)
        g_ptr_array_index(self->dir_stack, self->dir_stack->len - 1)
        ;

    g_ptr_array_remove_index (self->curr_comps, self->curr_comps->len - 1);

    if (self->dir_stack->len > 1)
    {
        /* 
         * If depth is false, then we no longer need the _curr_path
         * of the directories above the previously-set value, because we 
         * already traversed them.
         *
         * TODO : should this be if (! self->depth)
         */ 
        if (self->should_traverse_depth_first)
        {
            status_t status;

            status = file_finder_calc_curr_path(self);

            if (status != FILEFIND_STATUS_OK)
            {
                return status;
            }
        }
    }

    return FILEFIND_STATUS_OK;
}

static status_t file_finder_calc_default_actions(file_finder_t * self)
{
    int calc_obj = self->callback ? ACTION_RUN_CB : ACTION_SET_OBJ;

    if (self->should_traverse_depth_first)
    {
        self->def_actions[0] = ACTION_RECURSE;
        self->def_actions[1] = calc_obj;
    }
    else
    {
        self->def_actions[0] = calc_obj; 
        self->def_actions[1] = ACTION_RECURSE; 
    }

    return FILEFIND_STATUS_OK;
}

static void file_finder_fill_actions(
    file_finder_t * self,
    path_component_t * other
)
{
    memcpy(other->actions, self->def_actions, sizeof(other->actions));

    return;
}
