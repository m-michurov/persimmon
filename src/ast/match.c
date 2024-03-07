#include "match.h"

#include "common/macros.h"

static bool MatchExact(AstPatternExact pattern[static 1], AstNode const node) {
    if (node.Type != pattern->NodeType) {
        return false;
    }

    pattern->Base.MatchedNode = node;
    return true;
}

static void MatchRest(
        AstPatternRest pattern[static 1],
        size_t nodesCount,
        AstNode const nodes[static nodesCount]
) {
    if (nodesCount > 0) {
        pattern->Base.MatchedNode = nodes[0];
    }

    pattern->NodesCount = nodesCount;
    pattern->Nodes = nodes;
}

static bool MatchExpression(AstPatternExpression pattern[static 1], AstNode const node) {
    if (node.Type != AST_EXPRESSION) {
        return false;
    }

    auto const children = node.AsExpression.Items;

    size_t i = 0;
    Vector_ForEach(childPtr, children) {
        auto itemPattern = pattern->ItemPatterns[i];
        if (NULL == itemPattern) { return false; }

        if (AST_PATTERN_MATCH_REST == itemPattern->Type) {
            MatchRest((AstPatternRest *) itemPattern, children.Size - i, childPtr);
            i = children.Size;
            break;
        }

        if (false == Ast_MatchByType(itemPattern, children.Items[i])) {
            return false;
        }

        i++;
    }

    if (children.Size != i) {
        return false;
    }

    pattern->Base.MatchedNode = node;
    return true;
}

bool Ast_MatchByType(AstPattern *pattern, AstNode const node) {
    switch (pattern->Type) {
        case AST_PATTERN_MATCH_EXACT_TYPE:
            return MatchExact((AstPatternExact *) pattern, node);
        case AST_PATTERN_MATCH_ANY_TYPE: {
            pattern->MatchedNode = node;
            return true;
        }
        case AST_PATTERN_MATCH_REST:
            Unreachable("AST_PATTERN_MATCH_REST must not be used outside of expression");
        case AST_PATTERN_MATCH_EXPRESSION:
            return MatchExpression((AstPatternExpression *) pattern, node);
    }

    Unreachable("%d", pattern->Type);
}
