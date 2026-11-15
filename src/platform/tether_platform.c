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

void tether_platform_sync_ui(Tether_SyncCompleteFn on_complete, void *user_data) {
    if (on_complete) on_complete(user_data);
}

void tether_platform_sync_file(const char *rel_path, Tether_SyncCompleteFn on_complete, void *user_data) {
    (void)rel_path;
    if (on_complete) on_complete(user_data);
}

#else

void tether_platform_pick_file(const char *filter_ext, const char *title,
                               Tether_FilePickedFn on_picked, void *user_data) {
    (void)filter_ext; (void)title;
    if (on_picked) on_picked(NULL, user_data);
}

bool tether_platform_resolve_path(const char *in_path, char *out_path, size_t size) {
    if (!in_path || !out_path || size == 0) return false;
    return realpath(in_path, out_path) != NULL;
}

void tether_platform_sync_ui(Tether_SyncCompleteFn on_complete, void *user_data) {
    if (on_complete) on_complete(user_data);
}

void tether_platform_sync_file(const char *rel_path, Tether_SyncCompleteFn on_complete, void *user_data) {
    (void)rel_path;
    if (on_complete) on_complete(user_data);
}

#endif
