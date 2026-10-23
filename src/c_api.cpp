#include "c_api.h"

#include "logging.h"
#include "ocr_engine.h"

#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Init.h"
#include "plog/Log.h"

#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

struct ocr_engine
{
    OCR::OCREngine engine;
};

namespace
{

/* Initialize plog once, guarded for thread-safety by the C++11 static init. */
void EnsureLogging()
{
    static const bool initialized = []()
    {
        static plog::ConsoleAppender<plog::MyFormatter> appender;
        plog::init(plog::info, &appender);
        return true;
    }();
    (void)initialized;
}

char * DupString(const std::string &s)
{
    char *buffer = static_cast<char *>(std::malloc(s.size() + 1));
    if (buffer == nullptr)
    {
        return nullptr;
    }
    std::memcpy(buffer, s.c_str(), s.size() + 1);
    return buffer;
}

}   // unnamed namespace

extern "C"
{

ocr_engine * ocr_create(const char *config_path)
{
    if (config_path == nullptr)
    {
        return nullptr;
    }

    EnsureLogging();

    ocr_engine *handle = nullptr;
    try
    {
        handle = new ocr_engine();
    }
    catch (const std::bad_alloc &)
    {
        return nullptr;
    }

    if (!handle->engine.Initialize(config_path))
    {
        delete handle;
        return nullptr;
    }

    return handle;
}

void ocr_destroy(ocr_engine *engine)
{
    delete engine;
}

int ocr_run(ocr_engine *engine, const char *image_path,
            ocr_result **out_results, int *out_count)
{
    if (engine == nullptr || image_path == nullptr ||
        out_results == nullptr || out_count == nullptr)
    {
        return -1;
    }

    *out_results = nullptr;
    *out_count = 0;

    cv::Mat image;
    try
    {
        image = cv::imread(image_path, cv::IMREAD_COLOR_BGR);
    }
    catch (const std::exception &)
    {
        return -2;
    }

    if (image.empty())
    {
        return -3;
    }

    std::vector<OCR::OCRResult> results;
    try
    {
        results = engine->engine.Run(image);
    }
    catch (const std::exception &)
    {
        return -4;
    }

    if (results.empty())
    {
        return 0;
    }

    ocr_result *array = static_cast<ocr_result *>(
        std::calloc(results.size(), sizeof(ocr_result)));
    if (array == nullptr)
    {
        return -5;
    }

    for (size_t i = 0; i < results.size(); ++i)
    {
        const OCR::OCRResult &res = results[i];

        array[i].text = DupString(res.line.text);
        if (array[i].text == nullptr)
        {
            ocr_free_results(array, static_cast<int>(i));
            return -5;
        }

        array[i].score = res.angle.score;

        int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
        const auto &points = res.box.points;
        if (!points.empty())
        {
            min_x = max_x = points[0].x;
            min_y = max_y = points[0].y;
            for (const auto &pt : points)
            {
                min_x = std::min(min_x, pt.x);
                min_y = std::min(min_y, pt.y);
                max_x = std::max(max_x, pt.x);
                max_y = std::max(max_y, pt.y);
            }
        }

        array[i].x1 = min_x;
        array[i].y1 = min_y;
        array[i].x2 = max_x;
        array[i].y2 = max_y;
    }

    *out_results = array;
    *out_count = static_cast<int>(results.size());
    return 0;
}

void ocr_free_results(ocr_result *results, int count)
{
    if (results == nullptr)
    {
        return;
    }

    for (int i = 0; i < count; ++i)
    {
        std::free(results[i].text);
    }
    std::free(results);
}

}   // extern "C"
