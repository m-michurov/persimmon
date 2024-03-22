#pragma once

#include "call_checked.h"
#include "collections/vector.h"

#include "ast/ast.h"

typedef enum AstPatternType {
    AST_PATTERN_MATCH_EXACT_TYPE,
    AST_PATTERN_MATCH_ANY_TYPE,
    AST_PATTERN_MATCH_REST_EXACT_TYPE,
    AST_PATTERN_MATCH_REST_ANY_TYPE,
    AST_PATTERN_MATCH_EXPRESSION,
} AstPatternType;

typedef struct AstPattern AstPattern;

struct AstPattern {
    AstPatternType Type;
    AstNode MatchedNode;
};

typedef struct AstPatternByType AstPatternByType;
struct AstPatternByType {
    AstPattern Base;

    AstNodeType NodeType;
};

typedef VectorSlice_Of(AstNode) AstNodesSlice;

typedef struct AstPatternRestByType AstPatternRestByType;
struct AstPatternRestByType {
    AstPattern Base;

    AstNodeType NodeType;
    AstNodesSlice Nodes;
};

typedef struct AstPatternRest AstPatternRest;
struct AstPatternRest {
    AstPattern Base;

    AstNodesSlice Nodes;
};

typedef struct AstPatternExpression AstPatternExpression;
struct AstPatternExpression {
    AstPattern Base;

    AstPattern **ItemPatterns;
};

#define AstPattern_MatchAny()   ((AstPattern *) &(AstPattern) { .Type = AST_PATTERN_MATCH_ANY_TYPE, })
#define AstPattern_MatchRest()  ((AstPattern *) &(AstPatternRest) { .Base.Type = AST_PATTERN_MATCH_REST_ANY_TYPE, })

#define AstPattern_MatchRestByType(NodeType_)       \
((AstPattern *) &(AstPatternRestByType) {           \
    .Base.Type = AST_PATTERN_MATCH_REST_EXACT_TYPE, \
    .NodeType = NodeType_,                          \
})

#define AstPattern_MatchByType(NodeType_)       \
((AstPattern *) &(AstPatternByType) {           \
    .Base.Type = AST_PATTERN_MATCH_EXACT_TYPE,  \
    .NodeType = (NodeType_),                    \
})

#define AstPattern_MatchExpression(...)         \
(&(AstPatternExpression) {                      \
    .Base.Type = AST_PATTERN_MATCH_EXPRESSION,  \
    .ItemPatterns = (AstPattern *[]) {          \
        __VA_ARGS__, NULL                       \
    }                                           \
})

bool Ast_MatchByType(AstPattern *, AstNode);
