/**
 * \author Pyry Haulos <pyry.haulos@gmail.com>
 * \date 2007-2008
 * For conditions of distribution and use, see copyright notice in elfhacks.h
 */

#include <dlfcn.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "real_dlsym.h"
#include "elfhacks.h"

void *(*__dlopen)(const char *, int) = NULL;
void *(*__dlsym)(void *, const char *) = NULL;
char *(*__dlerror)(void) = NULL;
static bool print_dlopen;
static bool print_dlsym;

static void get_real_functions()
{
    eh_obj_t libdl;
    int ret;

    const char* libs[] = {
#if defined(__GLIBC__)
        "*libdl.so*",
#endif
        "*libc.so*",
        "*libc.*.so*",
        "*ld-musl-*.so*",
    };

    print_dlopen = getenv("MANGOHUD_DEBUG_DLOPEN") != NULL;
    print_dlsym = getenv("MANGOHUD_DEBUG_DLSYM") != NULL;

    for (size_t i = 0; i < sizeof(libs) / sizeof(*libs); i++)
    {
        ret = eh_find_obj(&libdl, libs[i]);
        if (ret)
            continue;

        eh_find_sym(&libdl, "dlopen", (void **) &__dlopen);
        eh_find_sym(&libdl, "dlsym", (void **) &__dlsym);
        eh_find_sym(&libdl, "dlerror", (void **) &__dlerror);
        eh_destroy_obj(&libdl);

        if (__dlopen && __dlsym && __dlerror)
            break;
        __dlopen = NULL;
        __dlsym = NULL;
        __dlerror = NULL;
    }

    if (!__dlopen && !__dlsym && !__dlerror)
    {
        fprintf(stderr, "MANGOHUD: Can't get dlopen(), dlsym() and dlerror()\n");
        exit(ret ? ret : 1);
    }
}

/**
 * \brief dlopen() wrapper, just passes calls to real dlopen()
 *        and writes information to standard output
 */
void *real_dlopen(const char *filename, int flag)
{
    if (__dlopen == NULL)
        get_real_functions();

    void *result = __dlopen(filename, flag);

    if (print_dlopen) {
        printf("dlopen(%s, ", filename);
        const char *fmt = "%s";
        #define FLAG(test) if (flag & test) { printf(fmt, #test); fmt = "|%s"; }
        FLAG(RTLD_LAZY)
        FLAG(RTLD_NOW)
        FLAG(RTLD_GLOBAL)
        FLAG(RTLD_LOCAL)
        FLAG(RTLD_NODELETE)
        FLAG(RTLD_NOLOAD)
#ifdef RTLD_DEEPBIND
        FLAG(RTLD_DEEPBIND)
#endif
        #undef FLAG
        printf(") = %p\n", result);
    }

    return result;
}

/**
 * \brief dlsym() wrapper, passes calls to real dlsym() and
 *        writes information to standard output
 */
void *real_dlsym(void *handle, const char *symbol)
{
    if (__dlsym == NULL)
        get_real_functions();

    void *result = __dlsym(handle, symbol);

    if (print_dlsym)
        printf("dlsym(%p, %s) = %p\n", handle, symbol, result);

    return result;
}

char* real_dlerror(void)
{
    if (__dlerror == NULL)
        get_real_functions();
    return __dlerror();
}

void* get_proc_address(const char* name) {
    return real_dlsym(RTLD_NEXT, name);
}
