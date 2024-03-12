#include "match.h"

#include "common/macros.h"

static bool MatchExact(AstPatternByType pattern[static 1], AstNode const node) {
    if (node.Type != pattern->NodeType) {
        return false;
    }

    pattern->Base.MatchedNode = node;
    return true;
}

static void MatchRest(AstPatternRest pattern[static 1], AstNodesSlice nodes) {
    if (false == Vector_Empty(nodes)) {
        pattern->Base.MatchedNode = nodes.Items[0];
    }

    pattern->Nodes = nodes;
}

static bool MatchRestExact(AstPatternRestByType pattern[static 1], AstNodesSlice nodes) {
    if (Vector_Any(nodes, it, pattern->NodeType != it.Type)) {
        return false;
    }

    if (nodes.Size > 0) {
        pattern->Base.MatchedNode = nodes.Items[0];
    }

    pattern->Nodes = nodes;
    return true;
}

static bool MatchExpression(AstPatternExpression pattern[static 1], AstNode const node) {
    if (node.Type != AST_EXPRESSION) {
        return false;
    }

    auto const expression = node.AsExpression;

    size_t i = 0;
    Vector_ForEach(childPtr, expression) {
        auto itemPattern = pattern->ItemPatterns[i];
        if (NULL == itemPattern) { return false; }

        if (AST_PATTERN_MATCH_REST_ANY_TYPE == itemPattern->Type) {
            MatchRest(
                    (AstPatternRest *) itemPattern,
                    Vector_SliceAs(AstNodesSlice, expression, i, expression.Size)
            );

            i = expression.Size;
            break;
        }

        if (AST_PATTERN_MATCH_REST_EXACT_TYPE == itemPattern->Type) {
            if (false == MatchRestExact(
                    (AstPatternRestByType *) itemPattern,
                    Vector_SliceAs(AstNodesSlice, expression, i, expression.Size)
            )) {
                return false;
            }

            i = expression.Size;
            break;
        }

        if (false == Ast_MatchByType(itemPattern, *childPtr)) {
            return false;
        }

        i++;
    }

    if (expression.Size != i) {
        return false;
    }

    pattern->Base.MatchedNode = node;
    return true;
}

bool Ast_MatchByType(AstPattern *pattern, AstNode const node) {
    switch (pattern->Type) {
        case AST_PATTERN_MATCH_EXACT_TYPE:
            return MatchExact((AstPatternByType *) pattern, node);
        case AST_PATTERN_MATCH_ANY_TYPE: {
            pattern->MatchedNode = node;
            return true;
        }
        case AST_PATTERN_MATCH_REST_ANY_TYPE:
            Unreachable("AST_PATTERN_MATCH_REST_ANY_TYPE must not be used outside of expression");
        case AST_PATTERN_MATCH_EXPRESSION:
            return MatchExpression((AstPatternExpression *) pattern, node);
        case AST_PATTERN_MATCH_REST_EXACT_TYPE:
            Unreachable("AST_PATTERN_MATCH_REST_EXACT_TYPE must not be used outside of expression");
    }

    Unreachable("%d", pattern->Type);
}
