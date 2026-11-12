#ifndef TETHER_YAML_WIDGETS_H
#define TETHER_YAML_WIDGETS_H

#include "tether/core/tether_ecs.h"
#include "tether/engine/parser/tether_yaml_ast.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Check if a widget name is a recognized built-in widget */
bool tether_yaml_is_builtin_widget(const char* name);

/* Apply widget-specific declarative properties from YAML AST to the entity */
bool tether_yaml_apply_widget_properties(Tether_GUID entity, const char* widget_name,
                                         Tether_ASTNode* props, Tether_ASTEnvironment* env);

#ifdef __cplusplus
}
#endif

#endif /* TETHER_YAML_WIDGETS_H */
