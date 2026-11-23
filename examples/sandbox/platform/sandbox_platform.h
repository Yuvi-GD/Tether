#ifndef SANDBOX_PLATFORM_H
#define SANDBOX_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Sandbox_FilePickedFn)(const char *path, void *user_data);
typedef void (*Sandbox_SyncCompleteFn)(void *user_data);

/* Asynchronously or synchronously pick a file using host platform dialogs. */
void sandbox_platform_pick_file(const char *filter_ext, const char *title,
                                Sandbox_FilePickedFn on_picked, void *user_data);

/* Resolves an input path to a canonical absolute path (for CLI args before chdir). */
bool sandbox_platform_resolve_path(const char *in_path, char *out_path, size_t size);

/* On web, re-fetches UI files from the web server into virtual MEMFS. Desktop: no-op. */
void sandbox_platform_sync_ui(Sandbox_SyncCompleteFn on_complete, void *user_data);

/* On web, re-fetches a single file from web server into virtual MEMFS. Desktop: no-op. */
void sandbox_platform_sync_file(const char *rel_path, Sandbox_SyncCompleteFn on_complete, void *user_data);

#if defined(__ANDROID__)
/* On Android, unpacks APK assets to internal storage and prepares sandbox directories. */
void sandbox_platform_android_init(void);
void sandbox_platform_android_poll(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* SANDBOX_PLATFORM_H */
