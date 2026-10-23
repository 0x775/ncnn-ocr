#ifndef OCR_C_API_H_
#define OCR_C_API_H_

/*
 * Stable C ABI for the OCREngine shared library (libpaddleocr.so).
 * This header contains only C-compatible declarations so that the
 * library can be consumed from C, C++, and other FFI-capable languages
 * without depending on the project's C++ ABI.
 */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#  if defined(OCR_BUILD_SHARED)
#    define OCR_API __declspec(dllexport)
#  else
#    define OCR_API __declspec(dllimport)
#  endif
#else
#  if defined(OCR_BUILD_SHARED)
#    define OCR_API __attribute__((visibility("default")))
#  else
#    define OCR_API
#  endif
#endif

/* Opaque handle to an OCR engine instance. */
typedef struct ocr_engine ocr_engine;

/* One recognized text box. `text` is owned by the result array and must
 * be released together with it via ocr_free_results(). */
typedef struct ocr_result
{
    char *text;   /* UTF-8, null-terminated */
    float score;  /* recognition confidence, 0.0 - 1.0 */

    /* axis-aligned bounding box in pixels, top-left / bottom-right */
    int x1;
    int y1;
    int x2;
    int y2;
} ocr_result;

/*
 * Create and initialize an engine from a JSON config file.
 * Returns NULL on failure.
 */
OCR_API ocr_engine * ocr_create(const char *config_path);

/*
 * Destroy an engine created by ocr_create().
 * Passing NULL is a no-op.
 */
OCR_API void ocr_destroy(ocr_engine *engine);

/*
 * Run OCR on an image file.
 * On success returns 0 and fills `out_results` / `out_count`.
 * The caller must free the results with ocr_free_results().
 * Returns non-zero on failure.
 */
OCR_API int ocr_run(ocr_engine *engine, const char *image_path,
                    ocr_result **out_results, int *out_count);

/*
 * Free a result array returned by ocr_run().
 * Passing (NULL, 0) is a no-op.
 */
OCR_API void ocr_free_results(ocr_result *results, int count);

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif  /* OCR_C_API_H_ */
