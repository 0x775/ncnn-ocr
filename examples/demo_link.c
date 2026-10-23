/*
 * Demo: link against libpaddleocr.so at build time.
 *
 * Build (x86):
 *   gcc demo_link.c -I../src -L../build/output/x64/lib -lpaddleocr -o demo_link
 * Run:
 *   LD_LIBRARY_PATH=../build/output/x64/lib ./demo_link ../config.json ../images/ocr_img1.png
 */
#include "c_api.h"

#include <stdio.h>
#include <time.h>

static double ElapsedMs(const struct timespec *start, const struct timespec *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0
        + (end->tv_nsec - start->tv_nsec) / 1e6;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <config.json> <image>\n", argv[0]);
        return 1;
    }

    struct timespec t0, t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    ocr_engine *engine = ocr_create(argv[1]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (engine == NULL)
    {
        fprintf(stderr, "Failed to create OCR engine with config: %s\n", argv[1]);
        return 2;
    }

    ocr_result *results = NULL;
    int count = 0;

    int ret = ocr_run(engine, argv[2], &results, &count);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    if (ret != 0)
    {
        fprintf(stderr, "ocr_run failed, code: %d\n", ret);
        ocr_destroy(engine);
        return 3;
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

    ocr_free_results(results, count);
    ocr_destroy(engine);
    return 0;
}
