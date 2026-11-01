#include "tool.h"
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
/* Windows argv quoting, including quotes and trailing backslashes. No shell. */
static bool append_arg(char *line, size_t capacity, const char *arg) {
    size_t used = strlen(line);
    if (strlen(arg) * 2 + used + 4 >= capacity) {
        return false;
    }
    if (used) {
        line[used++] = ' ';
    }
    line[used++] = '"';
    while (*arg) {
        size_t slashes = 0;
        while (*arg == '\\') {
            slashes++;
            arg++;
        }
        size_t copies = (*arg == '"' || !*arg) ? slashes * 2 : slashes;
        while (copies--) {
            line[used++] = '\\';
        }
        if (*arg == '"') {
            line[used++] = '\\';
        }
        if (*arg) {
            line[used++] = *arg++;
        }
    }
    line[used++] = '"';
    line[used] = '\0';
    return true;
}
#endif

bool tool_launch(ToolProcess *p, const char *name, const char *const *args) {
    if (p->active) {
        return false;
    }
    char directory[4096], executable[4096];
    if (!tool_executable_directory(directory, sizeof directory)) {
        return false;
    }
#ifdef _WIN32
    if (snprintf(executable, sizeof executable, "%s/%s.exe", directory, name) >=
        sizeof executable) {
        return false;
    }
    char command[16384] = "";
    if (!append_arg(command, sizeof command, executable)) {
        return false;
    }
    for (size_t i = 0; args && args[i]; i++) {
        if (!append_arg(command, sizeof command, args[i])) {
            return false;
        }
    }
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof startup;
    if (!CreateProcessA(executable, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, directory,
                        &startup, &process)) {
        return false;
    }
    CloseHandle(process.hThread);
    p->handle = process.hProcess;
    p->pid = (int)process.dwProcessId;
#else
    if (snprintf(executable, sizeof executable, "%s/%s", directory, name) >= sizeof executable) {
        return false;
    }
    char *arguments[64];
    size_t count = 0;
    arguments[count++] = executable;
    for (size_t i = 0; args && args[i]; i++) {
        if (count >= 63) {
            return false;
        }
        arguments[count++] = (char *)args[i];
    }
    arguments[count] = NULL;
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    }
    if (pid == 0) {
        if (chdir(directory)) {
            _exit(126);
        }
        execv(executable, arguments);
        _exit(127);
    }
    p->pid = (int)pid;
#endif
    p->active = true;
    return true;
}

bool tool_poll(ToolProcess *p, int *exit_code) {
    if (!p->active) {
        return false;
    }
#ifdef _WIN32
    if (WaitForSingleObject(p->handle, 0) == WAIT_TIMEOUT) {
        return false;
    }
    DWORD code = 1;
    GetExitCodeProcess(p->handle, &code);
    *exit_code = (int)code;
    CloseHandle(p->handle);
    p->handle = NULL;
#else
    int status;
    pid_t done = waitpid(p->pid, &status, WNOHANG);
    if (!done || (done < 0 && errno == EINTR)) {
        return false;
    }
    *exit_code = done < 0 ? 1 : (WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status));
#endif
    p->active = false;
    return true;
}

void tool_detach(ToolProcess *p) {
#ifdef _WIN32
    if (p->active) {
        CloseHandle(p->handle);
    }
#endif
    p->active = false;
    p->handle = NULL;
}
