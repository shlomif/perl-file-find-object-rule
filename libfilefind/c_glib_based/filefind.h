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
 *       Filename:  filefind.h
 *
 *    Description:  header file of libfilefind.
 *
 *        Created:  01/06/10 21:35:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shlomi Fish via Olivier Thauvin
 *
 * =========================================================================
 */

#ifndef FILEFIND_H
#define FILEFIND_H

enum FILE_FIND_IFACE_STATUS
{
    FILE_FIND_OK = 0,
    FILE_FIND_OUT_OF_MEMORY,
    FILE_FIND_END,
    FILE_FIND_COULD_NOT_OPEN_DIR,
};

typedef struct 
{
    int stub;
} file_find_handle_t;

extern int file_find_new(file_find_handle_t * * output_handle, const char * first_target);

extern void file_find_set_callback(
    file_find_handle_t * handle, 
    void (*callback)(const char * filename, void * context)
);

extern void file_find_set_should_traverse_depth_first(
    file_find_handle_t * handle,
    int should_traverse_depth_first
);

extern int file_find_next(file_find_handle_t * handle);

extern const char * file_find_get_path(file_find_handle_t * handle);

extern int file_find_set_traverse_to(
    file_find_handle_t * handle,
    int num_children,
    char * * children
);


extern int file_find_prune(
    file_find_handle_t * handle
);

extern int file_find_get_current_node_files_list(
    file_find_handle_t * handle,
    int * ptr_to_num_files,
    char * * * ptr_to_file_names
);

extern int file_find_get_traverse_to(
    file_find_handle_t * handle,
    int * ptr_to_num_files,
    char * * * ptr_to_file_names
);

extern int file_find_free(
    file_find_handle_t * handle
);

#endif /* #ifndef FILEFIND_H */
