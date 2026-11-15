#ifndef TETHER_PLATFORM_H
#define TETHER_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Tether_FilePickedFn)(const char *path, void *user_data);
typedef void (*Tether_SyncCompleteFn)(void *user_data);

/* Asynchronously or synchronously pick a file using the host platform's dialog.
 * filter_ext: Comma-separated extensions, e.g. "yaml,yml" or "json"
 * title: Dialog title or filter description (e.g. "YAML files (*.yaml;*.yml)")
 * on_picked: Callback called when a file is selected (or NULL if cancelled).
 * user_data: User context pointer passed to callback.
 */
void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data);

/* Resolves an input path to a canonical/absolute path. Returns true on success. */
bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size);

/* On web, re-fetches UI files from the web server into virtual MEMFS.
 * On desktop, immediately invokes on_complete.
 */
void tether_platform_sync_ui(Tether_SyncCompleteFn on_complete, void *user_data);

/* On web, re-fetches a single file from the web server into virtual MEMFS.
 * On desktop, immediately invokes on_complete.
 */
void tether_platform_sync_file(const char *rel_path, Tether_SyncCompleteFn on_complete, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_PLATFORM_H */
