#include "tether/engine/parser/tether_yaml_ast.h"
#include <stdio.h>
#include <string.h>

static bool write_input(const char *path, const char *contents) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }
    bool ok = fputs(contents, file) >= 0;
    if (fclose(file)) {
        ok = false;
    }
    return ok;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    const char *path = argv[1];
    unsigned failures = 0;
    for (unsigned i = 0; i < 20; i++) {
        if (!write_input(path, "- Panel:\n    size_box: [100, 50]\n    children:\n      - Text:\n  "
                               "        string: hello\n")) {
            return 2;
        }
        Tether_ASTNode *root = tether_ast_parse_file(path);
        if (!root || root->type != TETHER_AST_SEQUENCE || root->child_count != 1) {
            failures++;
        }
        tether_ast_free(root);
        if (!write_input(path, "- Panel: [unterminated")) {
            return 2;
        }
        root = tether_ast_parse_file(path);
        if (root) {
            fprintf(stderr, "Malformed YAML returned a partial tree on iteration %u\n", i);
            failures++;
        }
        tether_ast_free(root);
    }
    char nested[132];
    memset(nested, '[', 65);
    nested[65] = '0';
    memset(nested + 66, ']', 65);
    nested[131] = '\0';
    if (!write_input(path, nested)) {
        return 2;
    }
    Tether_ASTNode *too_deep = tether_ast_parse_file(path);
    if (too_deep) {
        failures++;
    }
    tether_ast_free(too_deep);
    if (tether_ast_parse_file(NULL)) {
        failures++;
    }
    if (remove(path)) {
        return 2;
    }
    Tether_ASTNode *missing = tether_ast_parse_file(path);
    if (missing) {
        failures++;
    }
    tether_ast_free(missing);
    printf("YAML parse/recovery: %s\n", failures ? "FAILED" : "PASSED");
    return failures ? 1 : 0;
}
