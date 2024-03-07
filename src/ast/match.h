#pragma once

#include "ast/ast.h"

typedef enum AstPatternType {
    AST_PATTERN_MATCH_EXACT_TYPE,
    AST_PATTERN_MATCH_ANY_TYPE,
    AST_PATTERN_MATCH_REST,
    AST_PATTERN_MATCH_EXPRESSION,
} AstPatternType;

typedef struct AstPattern AstPattern;

struct AstPattern {
    AstPatternType Type;
    AstNode MatchedNode;
};

typedef struct AstPatternExact AstPatternExact;
struct AstPatternExact {
    AstPattern Base;

    AstNodeType NodeType;
};

typedef struct AstPatternRest AstPatternRest;
struct AstPatternRest {
    AstPattern Base;

    size_t NodesCount;
    AstNode const *Nodes;
};

typedef struct AstPatternExpression AstPatternExpression;
struct AstPatternExpression {
    AstPattern Base;

    AstPattern **ItemPatterns;
};

#define AstPattern_Any()   ((AstPattern *) &(AstPattern) { .Type = AST_PATTERN_MATCH_ANY_TYPE, })
#define AstPattern_Rest()  ((AstPattern *) &(AstPatternRest) { .Base.Type = AST_PATTERN_MATCH_REST, })

#define AstPattern_Type(NodeType_)              \
((AstPattern *) &(AstPatternExact) {            \
    .Base.Type = AST_PATTERN_MATCH_EXACT_TYPE,  \
    .NodeType = (NodeType_),                    \
})

#define AstPattern_Expression(...)         \
(&(AstPatternExpression) {                      \
    .Base.Type = AST_PATTERN_MATCH_EXPRESSION,  \
    .ItemPatterns = (AstPattern *[]) {          \
        __VA_ARGS__, NULL                       \
    }                                           \
})

bool Ast_MatchByType(AstPattern *, AstNode);
