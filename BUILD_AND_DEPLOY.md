# 编译、测试与部署调用指南

本文档说明如何将本项目编译为动态链接库（`libpaddleocr.so`）、如何运行测试与示例，以及如何在其他程序中部署调用。

---

## 1. 目录结构概览

```
PaddleOCR-ncnn-CPP/
├── CMakeLists.txt
├── config.json                 # 运行配置（模型路径、阈值、线程数等）
├── models/                     # ncnn 模型（*.param + *.bin）与字典
├── images/                     # 测试图片
├── src/
│   ├── c_api.h                 # 对外 C ABI 头文件（部署方需要）
│   ├── c_api.cpp               # C ABI 实现
│   ├── ocr_engine.*            # 核心引擎
│   ├── db_net.* angle_net.* crnn_net.*
│   └── ...
└── examples/
    ├── demo_link.c             # C：编译期链接
    ├── demo_dlopen.c           # C：运行时 dlopen
    └── demo_cpp.cpp            # C++：RAII + 耗时
```

构建产物统一输出到：

```
build/<build-dir>/output/<platform>/
├── lib/
│   ├── libpaddleocr.so         # 动态库（对外交付）
│   └── libocr_core.a           # 内部静态库
└── bin/
    ├── main                    # 原可执行程序
    ├── demo_link
    ├── demo_dlopen
    └── demo_cpp
```

`<platform>` 由 `CMAKE_SYSTEM_PROCESSOR` 自动判定为 `x64` / `arm64` / `armhf`。

---

## 2. 依赖

| 依赖 | 说明 |
|---|---|
| CMake | >= 3.20 |
| C++17 编译器 | gcc / clang |
| OpenCV | 建议 4.x |
| ncnn | 需提供 `ncnnConfig.cmake` |
| OpenMP | 2.0+ |

依赖路径默认从 `$HOME/Documents/libs` 查找，也可通过 CMake 变量 `OpenCV_DIR`、`ncnn_DIR` 显式指定。

---

## 3. 编译

### 3.1 x86_64（本机原生编译）

```bash
cd PaddleOCR-ncnn-CPP

cmake -S . -B build/x64 \
  -DOpenCV_DIR=/usr/local/lib/cmake/opencv4 \
  -Dncnn_DIR=/opt/ncnn/build/install/lib/cmake/ncnn \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/x64 -j"$(nproc)"
```

产物位于 `build/x64/output/x64/`。

> 依赖装在默认的 `$HOME/Documents/libs` 下时，可省略 `-DOpenCV_DIR` / `-Dncnn_DIR`。

### 3.2 ARM（交叉编译）

> ARM 交叉编译需额外准备：aarch64/armhf 交叉工具链，以及对应架构的 OpenCV、ncnn 库。以下为 aarch64 示例。

1. 准备 toolchain 文件 `toolchain-aarch64.cmake`：

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# 目标架构依赖的根目录（自行调整）
set(CMAKE_FIND_ROOT_PATH /path/to/aarch64/sysroot)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

2. 配置与编译（交叉编译时 `examples/` 下的 demo 会自动跳过）：

```bash
cmake -S . -B build/arm64 \
  -DCMAKE_TOOLCHAIN_FILE=toolchain-aarch64.cmake \
  -DOpenCV_DIR=/path/to/aarch64/opencv/lib/cmake/opencv4 \
  -Dncnn_DIR=/path/to/aarch64/ncnn/lib/cmake/ncnn \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build/arm64 -j"$(nproc)"
```

产物位于 `build/arm64/output/arm64/`。

---

## 4. 测试

### 4.1 直接运行原程序

`config.json` 内模型路径为相对路径（`../models/...`），因此需在 `build` 目录下运行：

```bash
cd build
LD_LIBRARY_PATH=x64/output/x64/lib \
  ./x64/output/x64/bin/main ../config.json ../images/pinyin_demo.png
```

预期输出识别到的每行文本。

### 4.2 运行三个示例

```bash
cd build

# C：编译期链接
LD_LIBRARY_PATH=x64/output/x64/lib \
  ./x64/output/x64/bin/demo_link ../config.json ../images/pinyin_demo.png

# C：运行时 dlopen
./x64/output/x64/bin/demo_dlopen \
  ./x64/output/x64/lib/libpaddleocr.so \
  ../config.json ../images/pinyin_demo.png

# C++：RAII + 耗时
LD_LIBRARY_PATH=x64/output/x64/lib \
  ./x64/output/x64/bin/demo_cpp ../config.json ../images/pinyin_demo.png
```

每个示例会打印识别结果数量、逐条文本与置信度，并输出：

```
init: xx ms, inference: xx ms, total: xx ms
```

### 4.3 常见问题

**问题：所有识别文本为空，日志出现 `Unmatched scores: N != M`**

原因：字典与识别（rec）模型不匹配。rec 模型最后一层输出类别数必须等于字典行数（含首行 blank）。

排查：

```bash
# 查看 rec 模型输出类别数（gemm 层的 8= 参数）
grep "gemm_1" models/PP_OCRv6_tiny_rec.param

# 查看字典实际行数
awk 'END{print NR}' models/ppocr_keys_v6_tiny.txt
```

两者需一致，并确保 `config.json` 的 `rec.keys_path` 指向正确字典。

**问题：`Failed to load model`**

检查 `config.json` 中 `model_path` 指向的路径（不含后缀），实际需存在同名的 `.param` 与 `.bin`。

---

## 5. 部署与在其他程序中调用

### 5.1 需要交付的文件

```
libpaddleocr.so              # 动态库
c_api.h                      # 对外头文件
config.json                  # 运行配置
models/                      # 模型与字典
```

`libpaddleocr.so` 依赖系统或随包提供的 OpenCV、ncnn、OpenMP 运行库。

### 5.2 对外 C API

见 `src/c_api.h`，共 4 个函数：

```c
ocr_engine * ocr_create(const char *config_path);
void         ocr_destroy(ocr_engine *engine);
int          ocr_run(ocr_engine *engine, const char *image_path,
                     ocr_result **out_results, int *out_count);
void         ocr_free_results(ocr_result *results, int count);
```

`ocr_result` 字段：

```c
typedef struct ocr_result {
    char *text;   // UTF-8 文本
    float score;  // 置信度
    int x1, y1;   // 包围框左上
    int x2, y2;   // 包围框右下
} ocr_result;
```

约定：

- `ocr_create` 失败返回 `NULL`。
- `ocr_run` 成功返回 `0`，`out_results` / `out_count` 由库分配，**必须**用 `ocr_free_results` 释放。
- 所有句柄通过 `ocr_destroy` 释放。

### 5.3 调用方式一：编译期链接

```bash
gcc app.c -I/path/to/src \
  -L/path/to/lib -lpaddleocr \
  -o app

# 运行时指定库路径（或安装到系统库目录 / 配置 rpath）
LD_LIBRARY_PATH=/path/to/lib ./app config.json image.png
```

CMake 项目中：

```cmake
target_include_directories(app PRIVATE /path/to/src)
target_link_libraries(app PRIVATE paddleocr)   # 或 -L... -lpaddleocr
```

### 5.4 调用方式二：运行时 dlopen

适用于库路径在运行时才确定的场景：

```c
void *lib = dlopen("libpaddleocr.so", RTLD_NOW | RTLD_LOCAL);
ocr_engine *(*create)(const char *) = dlsym(lib, "ocr_create");
/* ... 解析 ocr_run / ocr_free_results / ocr_destroy 后调用 ... */
dlclose(lib);
```

完整示例见 `examples/demo_dlopen.c`。

### 5.5 C++ 调用

C API 可直接在 C++ 中使用，配合 RAII 管理资源，示例见 `examples/demo_cpp.cpp`。

### 5.6 其他语言

由于导出的是纯 C ABI，可通过各语言 FFI 调用：

- Python：`ctypes` / `cffi`
- Go：`cgo`
- Rust：`bindgen` + `libloading`

### 5.7 部署注意事项

- **库搜索路径**：将 `libpaddleocr.so` 放入系统库目录，或在程序中设置 `LD_LIBRARY_PATH`，或编译时加 `-Wl,-rpath,/path/to/lib`。
- **config 与 models 的相对路径**：`config.json` 中的模型路径相对于**运行工作目录**，部署时建议改为绝对路径或固定工作目录。
- **线程数**：`infer_threads` / `reco_threads` 设为 `-1` 表示自动，生产环境可按 CPU 核数显式配置。
- **FP16**：部分设备开启 FP16 可能产生 NaN，默认关闭。
