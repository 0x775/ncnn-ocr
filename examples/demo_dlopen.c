/*
 * Demo: load libpaddleocr.so at runtime via dlopen/dlsym.
 * Useful when the library path is only known at runtime.
 *
 * Build (x86):
 *   gcc demo_dlopen.c -I../src -ldl -o demo_dlopen
 * Run:
 *   ./demo_dlopen ./libpaddleocr.so ../config.json ../images/ocr_img1.png
 */
#include "c_api.h"

#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

static double ElapsedMs(const struct timespec *start, const struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0
        + (end->tv_nsec - start->tv_nsec) / 1e6;
}

typedef ocr_engine * (*fn_create)(const char *);
typedef void (*fn_destroy)(ocr_engine *);
typedef int (*fn_run)(ocr_engine *, const char *, ocr_result **, int *);
typedef void (*fn_free)(ocr_result *, int);

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <libpaddleocr.so> <config.json> <image>\n", argv[0]);
        return 1;
    }

    void *lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (lib == NULL)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 2;
    }

    fn_create create = (fn_create)dlsym(lib, "ocr_create");
    fn_destroy destroy = (fn_destroy)dlsym(lib, "ocr_destroy");
    fn_run run = (fn_run)dlsym(lib, "ocr_run");
    fn_free free_results = (fn_free)dlsym(lib, "ocr_free_results");

    if (create == NULL || destroy == NULL || run == NULL || free_results == NULL)
    {
        fprintf(stderr, "Failed to resolve symbols: %s\n", dlerror());
        dlclose(lib);
        return 3;
    }

    struct timespec t0, t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    ocr_engine *engine = create(argv[2]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (engine == NULL)
    {
        fprintf(stderr, "Failed to create OCR engine with config: %s\n", argv[2]);
        dlclose(lib);
        return 4;
    }

    ocr_result *results = NULL;
    int count = 0;

    int ret = run(engine, argv[3], &results, &count);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    if (ret != 0)
    {
        fprintf(stderr, "ocr_run failed, code: %d\n", ret);
        destroy(engine);
        dlclose(lib);
        return 5;
    }

    printf("Recognized %d text box(es):\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  [%d] \"%s\" (score: %.3f) box: (%d,%d)-(%d,%d)\n",
               i, results[i].text, results[i].score,
               results[i].x1, results[i].y1, results[i].x2, results[i].y2);
    }

    printf("init: %.3f ms, inference: %.3f ms, total: %.3f ms\n",
           ElapsedMs(&t0, &t1), ElapsedMs(&t1, &t2), ElapsedMs(&t0, &t2));

    free_results(results, count);
    destroy(engine);
    dlclose(lib);
    return 0;
}
