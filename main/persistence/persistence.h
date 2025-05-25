#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

namespace persist
{
    static inline bool load_struct(const char *path, void *obj, size_t size)
    {
        FILE *f = fopen(path, "rb");
        if (!f)
            return false;
        size_t n = fread(obj, 1, size, f);
        fclose(f);
        return n == size;
    }

    static inline bool save_struct(const char *path, const void *obj, size_t size)
    {
        FILE *f = fopen(path, "wb");
        if (!f)
            return false;
        size_t n = fwrite(obj, 1, size, f);
        fclose(f);
        return n == size;
    }

 

}