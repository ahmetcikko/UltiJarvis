#include "whisper_wrapper.h"
#include <whisper.h>

#ifdef _WIN32
#include <windows.h>
#include <cstdint>
#endif

static whisper_context *ctx = nullptr;

#ifdef _WIN32
static bool gpu_available() {
    HMODULE vk = LoadLibraryW(L"vulkan-1.dll");
    if (!vk)
        return false;
    struct VkApplicationInfoStub {
        std::uint32_t sType;
        const void *pNext;
        const char *pApplicationName;
        std::uint32_t applicationVersion;
        const char *pEngineName;
        std::uint32_t engineVersion;
        std::uint32_t apiVersion;
    };
    struct VkInstanceCreateInfoStub {
        std::uint32_t sType;
        const void *pNext;
        std::uint32_t flags;
        const VkApplicationInfoStub *pApplicationInfo;
        std::uint32_t enabledLayerCount;
        const char *const *ppEnabledLayerNames;
        std::uint32_t enabledExtensionCount;
        const char *const *ppEnabledExtensionNames;
    };
    using PFN_create = int(__stdcall *)(const VkInstanceCreateInfoStub *,
                                        const void *, void **);
    using PFN_enum = int(__stdcall *)(void *, std::uint32_t *, void **);
    using PFN_destroy = void(__stdcall *)(void *, const void *);
    auto create = reinterpret_cast<PFN_create>(
        reinterpret_cast<void *>(GetProcAddress(vk, "vkCreateInstance")));
    auto enumerate = reinterpret_cast<PFN_enum>(reinterpret_cast<void *>(
        GetProcAddress(vk, "vkEnumeratePhysicalDevices")));
    auto destroy = reinterpret_cast<PFN_destroy>(
        reinterpret_cast<void *>(GetProcAddress(vk, "vkDestroyInstance")));
    if (!create || !enumerate || !destroy) {
        FreeLibrary(vk);
        return false;
    }
    VkApplicationInfoStub appinfo {};
    appinfo.sType = 0;
    appinfo.apiVersion = (1u << 22);
    VkInstanceCreateInfoStub info {};
    info.sType = 1;
    info.pApplicationInfo = &appinfo;
    void *instance = nullptr;
    if (create(&info, nullptr, &instance) != 0 || !instance) {
        FreeLibrary(vk);
        return false;
    }
    std::uint32_t count = 0;
    int rc = enumerate(instance, &count, nullptr);
    destroy(instance, nullptr);
    FreeLibrary(vk);
    return rc == 0 && count > 0;
}
#else
static bool gpu_available() { return true; }
#endif

void whisper_init() {
    if (ctx)
        return;
    whisper_context_params params = whisper_context_default_params();
    params.use_gpu = gpu_available();
    ctx = whisper_init_from_file_with_params("ggml-small.en-q5_1.bin", params);
    if (!ctx && params.use_gpu) {
        params.use_gpu = false;
        ctx = whisper_init_from_file_with_params("ggml-small.en-q5_1.bin",
                                                 params);
    }
}

bool whisper_ready() { return ctx != nullptr; }

std::string transcribe(const float *samples, int n_samples) {
    if (!ctx)
        return "";

    // below half a second at 16kHz there's rarely anything worth decoding
    if (n_samples < 8000)
        return "";
    whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_BEAM_SEARCH);
    params.single_segment = true;
    params.no_context = true;
    params.no_timestamps = true;
    params.language = "en";
    if (whisper_full(ctx, params, samples, n_samples) != 0)
        return "";
    std::string result;
    int n = whisper_full_n_segments(ctx);
    for (int i = 0; i < n; i++)
        result += whisper_full_get_segment_text(ctx, i);
    return result;
}
