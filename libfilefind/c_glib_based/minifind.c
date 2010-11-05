#include <stdio.h>

#include "filefind.h"

int main(int argc, char * argv[])
{
    file_find_handle_t * tree;

    if (argc < 2)
    {
        fprintf(stderr, "%s\n", "Usage: minifind [path]");
        return -1;
    }

    if (file_find_new(&tree, argv[1]) != FILE_FIND_OK)
    {
        fprintf(stderr, "%s\n", "Could not allocate file finder.");
        return -1;
    }

    while (file_find_next(tree) == FILE_FIND_OK)
    {
        printf("%s\n", file_find_get_path(tree));
    }

    if (file_find_free(tree) != FILE_FIND_OK)
    {
        fprintf(stderr, "%s\n", "Not succesful in freeing the finder.");
        return -1;
    }

    return 0;
}
