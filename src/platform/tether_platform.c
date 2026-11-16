#include "tether/platform/tether_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>

static Tether_FilePickedFn s_web_picked_cb = NULL;
static void *s_web_picked_ud = NULL;
static char s_web_picked_path[1024] = {0};

static Tether_SyncCompleteFn s_sync_ui_cb = NULL;
static void *s_sync_ui_ud = NULL;

EMSCRIPTEN_KEEPALIVE char *tether_platform_get_path_buf(void) {
    return s_web_picked_path;
}

EMSCRIPTEN_KEEPALIVE void tether_platform_on_file_loaded(void) {
    if (s_web_picked_cb) {
        Tether_FilePickedFn cb = s_web_picked_cb;
        void *ud = s_web_picked_ud;
        s_web_picked_cb = NULL;
        s_web_picked_ud = NULL;
        cb(s_web_picked_path[0] ? s_web_picked_path : NULL, ud);
    }
}

EMSCRIPTEN_KEEPALIVE void tether_platform_on_sync_done(void) {
    if (s_sync_ui_cb) {
        Tether_SyncCompleteFn cb = s_sync_ui_cb;
        void *ud = s_sync_ui_ud;
        s_sync_ui_cb = NULL;
        s_sync_ui_ud = NULL;
        cb(ud);
    }
}

EM_JS(void, _tether_platform_js_pick_file, (const char *filter_ext), {
    var acceptStr = "";
    if (filter_ext) {
        var rawExt = (typeof UTF8ToString === "function") ? UTF8ToString(filter_ext) :
                     (typeof Module !== "undefined" && Module.UTF8ToString) ? Module.UTF8ToString(filter_ext) : "";
        if (rawExt) {
            acceptStr = rawExt.split(",").map(function(s) {
                s = s.trim();
                return s.startsWith(".") ? s : "." + s;
            }).join(",");
        }
    }
    var input = document.getElementById("tether_file_picker_input");
    if (!input) {
        input = document.createElement("input");
        input.type = "file";
        input.id = "tether_file_picker_input";
        input.style.display = "none";
        document.body.appendChild(input);
    }
    input.accept = acceptStr;
    input.value = "";
    input.onchange = function(e) {
        var file = e.target.files && e.target.files[0];
        if (!file) {
            var bufPtr = (typeof Module !== "undefined" && Module._tether_platform_get_path_buf) ?
                         Module._tether_platform_get_path_buf() : 0;
            if (bufPtr && typeof HEAPU8 !== "undefined") HEAPU8[bufPtr] = 0;
            if (typeof Module !== "undefined" && Module._tether_platform_on_file_loaded) {
                Module._tether_platform_on_file_loaded();
            }
            return;
        }
        var reader = new FileReader();
        reader.onload = function(evt) {
            try {
                var arrayBuffer = evt.target.result;
                var uint8 = new Uint8Array(arrayBuffer);
                var fs = (typeof FS !== "undefined") ? FS :
                         (typeof Module !== "undefined" && Module.FS) ? Module.FS : null;
                if (!fs) {
                    console.error("[Tether] Emscripten virtual filesystem (FS) is not available");
                    return;
                }
                try { fs.mkdir("/tmp"); } catch (ignore) {}
                var virtualPath = "/tmp/" + file.name;
                fs.writeFile(virtualPath, uint8);
                fs.writeFile("/tmp/chosen.yaml", uint8);
                console.log("[Tether] Picked file written to virtual FS: " + virtualPath + " (" + uint8.length + " bytes)");

                var bufPtr = (typeof Module !== "undefined" && Module._tether_platform_get_path_buf) ?
                             Module._tether_platform_get_path_buf() : 0;
                if (bufPtr) {
                    var stringToUTF8Fn = (typeof stringToUTF8 === "function") ? stringToUTF8 :
                                         (typeof Module !== "undefined" ? Module.stringToUTF8 : null);
                    if (stringToUTF8Fn) {
                        stringToUTF8Fn(virtualPath, bufPtr, 1024);
                    }
                }
                if (typeof Module !== "undefined" && Module._tether_platform_on_file_loaded) {
                    Module._tether_platform_on_file_loaded();
                }
            } catch (err) {
                console.error("[Tether] Error loading picked file into virtual FS:", err);
            }
        };
        reader.onerror = function(err) {
            console.error("[Tether] FileReader error:", err);
            var bufPtr = (typeof Module !== "undefined" && Module._tether_platform_get_path_buf) ?
                         Module._tether_platform_get_path_buf() : 0;
            if (bufPtr && typeof HEAPU8 !== "undefined") HEAPU8[bufPtr] = 0;
            if (typeof Module !== "undefined" && Module._tether_platform_on_file_loaded) {
                Module._tether_platform_on_file_loaded();
            }
        };
        reader.readAsArrayBuffer(file);
    };
    input.click();
});

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    (void)title;
    s_web_picked_cb = on_picked;
    s_web_picked_ud = user_data;
    s_web_picked_path[0] = '\0';
    _tether_platform_js_pick_file(filter_ext);
}

bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size) {
    if (!in_path || !out_path || size == 0) return false;
    snprintf(out_path, size, "%s", in_path);
    return true;
}

EM_JS(void, _tether_platform_js_sync_all_ui, (), {
    var files = window.TETHER_SYNC_FILES || (typeof Module !== "undefined" && Module.tetherSyncFiles) || [
        "UI/app.yaml",
        "UI/widgets.yaml",
        "UI/dashboard.yaml",
        "UI/index.yaml",
        "UI/lab.yaml",
        "UI/playground.yaml",
        "UI/playground/layout.yaml",
        "UI/showcase_dialog.yaml",
        "UI/showcase_toast.yaml"
    ];
    var fs = (typeof FS !== "undefined") ? FS :
             (typeof Module !== "undefined" && Module.FS) ? Module.FS : null;
    if (!fs || !files || files.length === 0) {
        if (typeof Module !== "undefined" && Module._tether_platform_on_sync_done) {
            Module._tether_platform_on_sync_done();
        }
        return;
    }
    var pending = files.length;
    var timestamp = Date.now();
    files.forEach(function(f) {
        fetch(f + "?t=" + timestamp, { cache: "no-store" })
            .then(function(res) {
                if (res.ok) return res.arrayBuffer();
                return null;
            })
            .then(function(buf) {
                if (buf && fs) {
                    try {
                        fs.writeFile("/" + f, new Uint8Array(buf));
                        console.log("[Tether] Re-fetched " + f + " from server (" + buf.byteLength + " bytes)");
                    } catch (e) {
                        console.warn("[Tether] Error writing synced file " + f, e);
                    }
                }
            })
            .catch(function(err) {
                console.warn("[Tether] Failed to sync " + f, err);
            })
            .finally(function() {
                pending--;
                if (pending === 0) {
                    console.log("[Tether] UI sync complete.");
                    if (typeof Module !== "undefined" && Module._tether_platform_on_sync_done) {
                        Module._tether_platform_on_sync_done();
                    }
                }
            });
    });
});

void tether_platform_sync_ui(Tether_SyncCompleteFn on_complete, void *user_data) {
    if (!on_complete) return;
    s_sync_ui_cb = on_complete;
    s_sync_ui_ud = user_data;
    _tether_platform_js_sync_all_ui();
}

EM_JS(void, _tether_platform_js_sync_single_file, (const char *rel_path_ptr), {
    var pathStr = (typeof UTF8ToString === "function") ? UTF8ToString(rel_path_ptr) :
                  (typeof Module !== "undefined" && Module.UTF8ToString) ? Module.UTF8ToString(rel_path_ptr) : "";
    if (!pathStr) {
        if (typeof Module !== "undefined" && Module._tether_platform_on_sync_done) {
            Module._tether_platform_on_sync_done();
        }
        return;
    }
    var fs = (typeof FS !== "undefined") ? FS :
             (typeof Module !== "undefined" && Module.FS) ? Module.FS : null;
    fetch(pathStr + "?t=" + Date.now(), { cache: "no-store" })
        .then(function(res) {
            if (res.ok) return res.arrayBuffer();
            return null;
        })
        .then(function(buf) {
            if (buf && fs) {
                var target = pathStr.startsWith("/") ? pathStr : "/" + pathStr;
                fs.writeFile(target, new Uint8Array(buf));
                console.log("[Tether] Synced single file: " + target + " (" + buf.byteLength + " bytes)");
            }
        })
        .catch(function(e) {
            console.warn("[Tether] Failed to sync " + pathStr, e);
        })
        .finally(function() {
            if (typeof Module !== "undefined" && Module._tether_platform_on_sync_done) {
                Module._tether_platform_on_sync_done();
            }
        });
});

void tether_platform_sync_file(const char *rel_path, Tether_SyncCompleteFn on_complete, void *user_data) {
    if (!rel_path || !rel_path[0]) {
        if (on_complete) on_complete(user_data);
        return;
    }
    s_sync_ui_cb = on_complete;
    s_sync_ui_ud = user_data;
    _tether_platform_js_sync_single_file(rel_path);
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    if (!on_picked) return;
    char path[4096] = {0};
    char filter[1024] = {0};

    int pos = snprintf(filter, sizeof(filter), "%s", title ? title : "Files");
    pos++; /* skip null terminator */

    if (filter_ext && filter_ext[0]) {
        char ext_copy[256];
        snprintf(ext_copy, sizeof(ext_copy), "%s", filter_ext);
        char *token = strtok(ext_copy, ",");
        bool first = true;
        while (token && (size_t)pos < sizeof(filter) - 8) {
            while (*token == ' ' || *token == '.') token++;
            if (!first) {
                filter[pos++] = ';';
            }
            first = false;
            pos += snprintf(filter + pos, sizeof(filter) - (size_t)pos, "*.%s", token);
            token = strtok(NULL, ",");
        }
    } else {
        pos += snprintf(filter + pos, sizeof(filter) - (size_t)pos, "*.*");
    }
    pos++;
    filter[pos] = '\0'; /* double null terminator */

    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        on_picked(path, user_data);
    } else {
        on_picked(NULL, user_data);
    }
}

bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size) {
    if (!in_path || !out_path || size == 0) return false;
    return _fullpath(out_path, in_path, size) != NULL;
}

#elif defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <unistd.h>

static bool s_run_dialog_cmd(const char *cmd, char *out_path, size_t out_size) {
    if (!cmd || !cmd[0] || !out_path || out_size == 0) return false;
    FILE *fp = popen(cmd, "r");
    if (!fp) return false;
    bool found = false;
    if (fgets(out_path, (int)out_size, fp)) {
        size_t len = strlen(out_path);
        while (len > 0 && (out_path[len - 1] == '\n' || out_path[len - 1] == '\r')) {
            out_path[--len] = '\0';
        }
        found = (len > 0);
    }
    pclose(fp);
    return found;
}

bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size) {
    if (!in_path || !out_path || size == 0) return false;
    return realpath(in_path, out_path) != NULL;
}

#if defined(__APPLE__)

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    if (!on_picked) return;
    char path[4096] = {0};
    char cmd[1024] = {0};
    char apple_types[256] = {0};

    if (filter_ext && filter_ext[0]) {
        char ext_copy[256];
        snprintf(ext_copy, sizeof(ext_copy), "%s", filter_ext);
        char *token = strtok(ext_copy, ",");
        int pos = snprintf(apple_types, sizeof(apple_types), "of type {");
        bool first = true;
        while (token && (size_t)pos < sizeof(apple_types) - 8) {
            while (*token == ' ' || *token == '.') token++;
            pos += snprintf(apple_types + pos, sizeof(apple_types) - (size_t)pos,
                            "%s\"%s\"", first ? "" : ", ", token);
            first = false;
            token = strtok(NULL, ",");
        }
        snprintf(apple_types + pos, sizeof(apple_types) - (size_t)pos, "}");
    }

    snprintf(cmd, sizeof(cmd),
             "osascript -e 'POSIX path of (choose file %s with prompt \"%s\")' 2>/dev/null",
             apple_types, title ? title : "Choose File");

    if (s_run_dialog_cmd(cmd, path, sizeof(path))) {
        on_picked(path, user_data);
    } else {
        on_picked(NULL, user_data);
    }
}

#else /* Linux and Unix BSDs */

static bool s_has_command(const char *tool) {
    char check_cmd[128];
    snprintf(check_cmd, sizeof(check_cmd), "command -v %s >/dev/null 2>&1", tool);
    return system(check_cmd) == 0;
}

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    if (!on_picked) return;
    char path[4096] = {0};
    char cmd[1024] = {0};

    const char *desktop = getenv("XDG_CURRENT_DESKTOP");
    bool is_kde = desktop && (strstr(desktop, "KDE") || strstr(desktop, "plasma") || strstr(desktop, "Plasma"));

    char pattern_list[256] = {0};
    char zenity_filter[512] = {0};
    if (filter_ext && filter_ext[0]) {
        char ext_copy[256];
        snprintf(ext_copy, sizeof(ext_copy), "%s", filter_ext);
        char *token = strtok(ext_copy, ",");
        int pos = 0;
        while (token && (size_t)pos < sizeof(pattern_list) - 8) {
            while (*token == ' ' || *token == '.') token++;
            pos += snprintf(pattern_list + pos, sizeof(pattern_list) - (size_t)pos,
                            "%s*.%s", pos > 0 ? " " : "", token);
            token = strtok(NULL, ",");
        }
        snprintf(zenity_filter, sizeof(zenity_filter), "--file-filter=\"%s | %s\"",
                 title ? title : "Supported files", pattern_list);
    }

    const char *dlg_title = title ? title : "Open File";
    if (is_kde && s_has_command("kdialog")) {
        snprintf(cmd, sizeof(cmd),
                 "kdialog --title \"%s\" --getopenfilename . \"%s\" 2>/dev/null",
                 dlg_title, pattern_list[0] ? pattern_list : "*.*");
    } else if (s_has_command("zenity")) {
        snprintf(cmd, sizeof(cmd),
                 "zenity --file-selection --title=\"%s\" %s 2>/dev/null",
                 dlg_title, zenity_filter);
    } else if (s_has_command("kdialog")) {
        snprintf(cmd, sizeof(cmd),
                 "kdialog --title \"%s\" --getopenfilename . \"%s\" 2>/dev/null",
                 dlg_title, pattern_list[0] ? pattern_list : "*.*");
    } else if (s_has_command("qarma")) {
        snprintf(cmd, sizeof(cmd),
                 "qarma --file-selection --title=\"%s\" %s 2>/dev/null",
                 dlg_title, zenity_filter);
    } else if (s_has_command("yad")) {
        snprintf(cmd, sizeof(cmd),
                 "yad --file --title=\"%s\" 2>/dev/null",
                 dlg_title);
    }

    if (cmd[0] && s_run_dialog_cmd(cmd, path, sizeof(path))) {
        on_picked(path, user_data);
    } else {
        on_picked(NULL, user_data);
    }
}

#endif /* __APPLE__ vs Linux/BSD */

#else /* Generic Fallback */

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    (void)filter_ext;
    (void)title;
    if (on_picked) on_picked(NULL, user_data);
}

bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size) {
    if (!in_path || !out_path || size == 0) return false;
    snprintf(out_path, size, "%s", in_path);
    return true;
}

#endif

#if !defined(__EMSCRIPTEN__)
void tether_platform_sync_ui(Tether_SyncCompleteFn on_complete, void *user_data) {
    if (on_complete) on_complete(user_data);
}

void tether_platform_sync_file(const char *rel_path, Tether_SyncCompleteFn on_complete, void *user_data) {
    (void)rel_path;
    if (on_complete) on_complete(user_data);
}
#endif
