/*
 * Demo: use libpaddleocr.so from C++ with RAII wrappers.
 * Shows that the C ABI is directly consumable from C++, while keeping
 * resource cleanup exception-safe. Also reports timing breakdown.
 *
 * Build (x86, via CMake): produces demo_cpp under output/x64/bin/
 * Run:
 *   LD_LIBRARY_PATH=<libdir> ./demo_cpp ../config.json ../images/pinyin_demo.png
 */
#include "c_api.h"

#include <chrono>
#include <iostream>
#include <memory>

namespace
{

/* RAII deleter for the opaque engine handle. */
struct EngineDeleter
{
    void operator () (ocr_engine *engine) const noexcept
    {
        ocr_destroy(engine);
    }
};

/* RAII owner for a result array returned by ocr_run(). */
struct ResultBuffer
{
    ocr_result *data{nullptr};
    int count{0};

    ResultBuffer() = default;
    ResultBuffer(const ResultBuffer &) = delete;
    ResultBuffer & operator = (const ResultBuffer &) = delete;

    ~ResultBuffer()
    {
        if (data != nullptr)
        {
            ocr_free_results(data, count);
        }
    }
};

double ElapsedMs(const std::chrono::steady_clock::time_point &start)
{
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

}   // unnamed namespace

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <config.json> <image>\n";
        return 1;
    }

    /* 1. initialize engine and measure cost */
    const auto init_start = std::chrono::steady_clock::now();
    const std::unique_ptr<ocr_engine, EngineDeleter> engine(ocr_create(argv[1]));
    const double init_ms = ElapsedMs(init_start);

    if (!engine)
    {
        std::cerr << "Failed to create OCR engine with config: " << argv[1] << "\n";
        return 2;
    }

    /* 2. run inference and measure cost */
    ResultBuffer results;
    const auto run_start = std::chrono::steady_clock::now();
    const int ret = ocr_run(engine.get(), argv[2], &results.data, &results.count);
    const double run_ms = ElapsedMs(run_start);

    if (ret != 0)
    {
        std::cerr << "ocr_run failed, code: " << ret << "\n";
        return 3;
    }

    std::cout << "Recognized " << results.count << " text box(es):\n";
    for (int i = 0; i < results.count; ++i)
    {
        const ocr_result &res = results.data[i];
        std::cout << "  [" << i << "] \"" << res.text << "\" (score: " << res.score
                  << ") box: (" << res.x1 << "," << res.y1 << ")-("
                  << res.x2 << "," << res.y2 << ")\n";
    }

    std::cout << "init: " << init_ms << " ms, inference: " << run_ms
              << " ms, total: " << (init_ms + run_ms) << " ms\n";

    return 0;
}
