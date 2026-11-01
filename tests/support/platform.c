#include "tool.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <direct.h>
#include <commdlg.h>
#else
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach/mach.h>
#endif
#endif

double tool_now_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER counter, frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
}

uint32_t tool_process_id(void) {
#ifdef _WIN32
    return (uint32_t)GetCurrentProcessId();
#else
    return (uint32_t)getpid();
#endif
}

double tool_cpu_ms(void) {
#ifdef _WIN32
    FILETIME created, exited, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) {
        return NAN;
    }
    ULARGE_INTEGER k, u;
    k.LowPart = kernel.dwLowDateTime;
    k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime;
    u.HighPart = user.dwHighDateTime;
    return (k.QuadPart + u.QuadPart) / 10000.0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) {
        return NAN;
    }
    return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000.0 +
           (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000.0;
#endif
}

uint64_t tool_rss(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS usage;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &usage, sizeof usage)) {
        return UINT64_MAX;
    }
    return (uint64_t)usage.WorkingSetSize;
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) !=
        KERN_SUCCESS) {
        return UINT64_MAX;
    }
    return info.resident_size;
#else
    FILE *f = fopen("/proc/self/statm", "r");
    unsigned long pages = 0, resident = 0;
    if (!f) {
        return UINT64_MAX;
    }
    int n = fscanf(f, "%lu %lu", &pages, &resident);
    fclose(f);
    long page_size = sysconf(_SC_PAGESIZE);
    return n == 2 && page_size > 0 ? (uint64_t)resident * (uint64_t)page_size : UINT64_MAX;
#endif
}

uint64_t tool_available_memory(void) {
#ifdef _WIN32
    MEMORYSTATUSEX memory = {0};
    memory.dwLength = sizeof memory;
    return GlobalMemoryStatusEx(&memory) ? memory.ullAvailPhys : 0;
#elif defined(_SC_AVPHYS_PAGES)
    long pages = sysconf(_SC_AVPHYS_PAGES), size = sysconf(_SC_PAGESIZE);
    return pages > 0 && size > 0 ? (uint64_t)pages * (uint64_t)size : 0;
#else
    return 0;
#endif
}

void tool_environment(FILE *f) {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    char processor[256] = "";
    DWORD bytes = sizeof processor;
    const char *cpu = getenv("PROCESSOR_IDENTIFIER");
    if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                     "ProcessorNameString", RRF_RT_REG_SZ, NULL, processor,
                     &bytes) == ERROR_SUCCESS) {
        cpu = processor;
    }
    fputs("\"os\":\"Windows\",\"cpu\":", f);
    tool_json_string(f, cpu ? cpu : "unknown");
    typedef LONG(WINAPI * VersionFn)(OSVERSIONINFOW *);
    VersionFn version_fn =
        (VersionFn)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    OSVERSIONINFOW version = {0};
    version.dwOSVersionInfoSize = sizeof version;
    if (version_fn && version_fn(&version) == 0) {
        fprintf(f, ",\"os_release\":\"%lu.%lu.%lu\"", version.dwMajorVersion,
                version.dwMinorVersion, version.dwBuildNumber);
    } else {
        fputs(",\"os_release\":null", f);
    }
    fprintf(f, ",\"logical_processors\":%lu", info.dwNumberOfProcessors);
    DISPLAY_DEVICEA display = {0};
    display.cb = sizeof display;
    fputs(",\"display_adapter\":", f);
    tool_json_string(f,
                     EnumDisplayDevicesA(NULL, 0, &display, 0) ? display.DeviceString : "unknown");
    fputs(",\"gpu_device_note\":\"Display adapter identity; actual rendering device and driver "
          "version not queried\"",
          f);
#else
    struct utsname name;
    fputs("\"os\":", f);
    if (!uname(&name)) {
        tool_json_string(f, name.sysname);
        fputs(",\"os_release\":", f);
        tool_json_string(f, name.release);
    } else {
        tool_json_string(f, "unknown");
    }
    fprintf(f, ",\"logical_processors\":%ld", sysconf(_SC_NPROCESSORS_ONLN));
    fputs(",\"cpu\":null,\"display_adapter\":null", f);
#endif
#ifdef _WIN32
    MEMORYSTATUSEX memory = {0};
    memory.dwLength = sizeof memory;
    if (GlobalMemoryStatusEx(&memory)) {
        fprintf(f, ",\"physical_ram_bytes\":%llu,\"available_ram_at_report_bytes\":%llu",
                (unsigned long long)memory.ullTotalPhys, (unsigned long long)memory.ullAvailPhys);
    } else {
        fputs(",\"physical_ram_bytes\":null,\"available_ram_at_report_bytes\":null", f);
    }
#else
    fputs(",\"physical_ram_bytes\":null,\"available_ram_at_report_bytes\":null", f);
#endif
    fputs(",\"gpu_driver\":null,\"gpu_timing_supported\":false", f);
}

bool tool_executable_directory(char *out, size_t size) {
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)size);
    if (!n || n >= size) {
        return false;
    }
#elif defined(__APPLE__)
    uint32_t n = (uint32_t)size;
    if (_NSGetExecutablePath(out, &n)) {
        return false;
    }
#else
    ssize_t n = readlink("/proc/self/exe", out, size - 1);
    if (n < 0 || (size_t)n >= size - 1) {
        return false;
    }
    out[n] = '\0';
#endif
    char *slash = strrchr(out, '/');
    char *backslash = strrchr(out, '\\');
    if (backslash && (!slash || backslash > slash)) {
        slash = backslash;
    }
    if (!slash) {
        return false;
    }
    *slash = '\0';
    return true;
}

bool tool_enter_executable_directory(void) {
    char dir[4096];
    if (!tool_executable_directory(dir, sizeof dir)) {
        return false;
    }
#ifdef _WIN32
    return _chdir(dir) == 0;
#else
    return chdir(dir) == 0;
#endif
}

bool tool_make_results_directory(const char *path) {
    if (!path || !path[0]) {
        return false;
    }
#ifdef _WIN32
    if (CreateDirectoryA(path, NULL)) {
        return true;
    }
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
    if (!mkdir(path, 0755)) {
        return true;
    }
    struct stat entry;
    return errno == EEXIST && !stat(path, &entry) && S_ISDIR(entry.st_mode);
#endif
}

bool tool_make_output_directory(const char *output) {
    if (!output) {
        return false;
    }
    char path[4096];
    size_t length = strlen(output);
    if (length >= sizeof path) {
        return false;
    }
    memcpy(path, output, length + 1);
    char *last = NULL;
#ifdef _WIN32
    char *unc_root_end = NULL;
    if ((path[0] == '\\' || path[0] == '/') && path[1] == path[0]) {
        char *server_end = strpbrk(path + 2, "\\/");
        if (!server_end || server_end == path + 2 || !server_end[1]) {
            return false;
        }
        unc_root_end = strpbrk(server_end + 1, "\\/");
    }
#endif
    for (char *p = path; *p; p++) {
        if (*p == '/'
#ifdef _WIN32
            || *p == '\\'
#endif
        ) {
            last = p;
        }
    }
    if (!last) {
        return true;
    }
    if (last == path) {
        return true;
    }
#ifdef _WIN32
    if (last == path + 2 && path[1] == ':') {
        return true;
    }
#endif
    *last = '\0';
    for (char *p = path; *p; p++) {
        if (*p != '/'
#ifdef _WIN32
            && *p != '\\'
#endif
        ) {
            continue;
        }
        char separator = *p;
        *p = '\0';
        bool skip = p == path;
#ifdef _WIN32
        skip = skip || (p == path + 2 && path[1] == ':') || (unc_root_end && p <= unc_root_end);
#endif
        if (!skip && !tool_make_results_directory(path)) {
            return false;
        }
        *p = separator;
    }
    return tool_make_results_directory(path);
}

bool tool_replace_file(const char *source, const char *destination) {
#ifdef _WIN32
    return MoveFileExA(source, destination, MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return rename(source, destination) == 0;
#endif
}

bool tool_pick_result(char *path, size_t size) {
#ifdef _WIN32
    if (!size || size > 0xffffffffu) {
        return false;
    }
    path[0] = 0;
    OPENFILENAMEA dialog = {0};
    dialog.lStructSize = sizeof dialog;
    dialog.lpstrFilter = "Tether results (*.json)\0*.json\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = (DWORD)size;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&dialog) != 0;
#else
    (void)path;
    (void)size;
    return false;
#endif
}
