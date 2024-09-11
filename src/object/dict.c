#include "dict.h"

#include "utility/guards.h"
#include "utility/option.h"
#include "compare.h"
#include "constructors.h"

static ObjectOption next_min_node(Object *dict, ObjectOption prev_min_key) { // NOLINT(*-no-recursion)
    guard_is_not_null(dict);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return option_none(ObjectOption);
    }

    if (prev_min_key.has_value && object_compare(dict->as_dict.key, prev_min_key.value) <= 0) {
        return next_min_node(dict->as_dict.right, prev_min_key);
    }

    auto const left_min = next_min_node(dict->as_dict.left, prev_min_key);
    return option_or_some(left_min, dict);
}

static Object_CompareResult compare_nodes(Object *a, Object *b) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_equal(a->type, TYPE_DICT);
    guard_is_equal(b->type, TYPE_DICT);

    auto const key_compare_result = object_compare(a->as_dict.key, b->as_dict.key);
    if (OBJECT_EQUALS != key_compare_result) {
        return key_compare_result;
    }

    return object_compare(a->as_dict.value, b->as_dict.value);
}

static Object_CompareResult compare_position(ObjectOption prev_key, Object *cur_node, Object *dict) {
    guard_is_not_null(cur_node);
    guard_is_not_null(dict);

    auto const min_node = next_min_node(dict, prev_key);
    if (false == min_node.has_value) {
        return OBJECT_GREATER;
    }

    return compare_nodes(cur_node, min_node.value);
}

static Object_CompareResult compare( // NOLINT(*-no-recursion)
        Object *a,
        Object *b,
        ObjectOption prev_min_key,
        Object **next_min_node
) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_not_null(next_min_node);
    guard_is_equal(a->type, TYPE_DICT);
    guard_is_equal(b->type, TYPE_DICT);

    ObjectOption prev_key;
    if (object_nil() != a->as_dict.left) {
        Object *left_min_node;
        auto const result = compare(a->as_dict.left, b, prev_min_key, &left_min_node);
        if (OBJECT_EQUALS != result) {
            return result;
        }

        prev_key = option_some(ObjectOption, left_min_node->as_dict.key);
    } else {
        prev_key = prev_min_key;
    }

    auto const result = compare_position(prev_key, a, b);
    if (OBJECT_EQUALS != result) {
        return result;
    }

    if (object_nil() != a->as_dict.right) {
        return compare(a->as_dict.right, b, option_some(ObjectOption, a->as_dict.key), next_min_node);
    }

    *next_min_node = a;
    return OBJECT_EQUALS;
}

Object_CompareResult object_dict_compare(Object *a, Object *b) {
    guard_is_not_null(a);
    guard_is_not_null(b);
    guard_is_one_of(a->type, TYPE_NIL, TYPE_DICT);
    guard_is_one_of(b->type, TYPE_NIL, TYPE_DICT);

    auto const a_size = object_dict_size(a);
    auto const b_size = object_dict_size(b);

    if (a_size != b_size) {
        return a_size > b_size ? OBJECT_GREATER : OBJECT_LESS;
    }

    Object *unused;
    return compare(a, b, option_none(ObjectOption), &unused);
}

static int64_t height(Object *root) { // NOLINT(*-no-recursion)
    guard_is_one_of(root->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == root) {
        return 0;
    }

    return 1 + max(height(root->as_dict.left), height(root->as_dict.right));
}

static int64_t balance_factor(Object *root) {
    guard_is_one_of(root->type, TYPE_DICT);

    return height(root->as_dict.right) - height(root->as_dict.left);
}

static Object *rotate_right(Object *p) {
    guard_is_equal(p->type, TYPE_DICT);

    auto const q = p->as_dict.left;
    guard_is_equal(q->type, TYPE_DICT);

    p->as_dict.left = q->as_dict.right;
    q->as_dict.right = p;

    return q;
}

static Object *rotate_left(Object *q) {
    guard_is_equal(q->type, TYPE_DICT);

    auto const p = q->as_dict.right;
    guard_is_equal(p->type, TYPE_DICT);

    q->as_dict.right = p->as_dict.left;
    p->as_dict.left = q;

    return p;
}

static Object *balance(Object *p) {
    guard_is_equal(p->type, TYPE_DICT);

    auto const factor = balance_factor(p);

    if (2 == factor) {
        if (balance_factor(p->as_dict.right) < 0) {
            p->as_dict.right = rotate_right(p->as_dict.right);
        }

        return rotate_left(p);
    }

    if (-2 == factor) {
        if (balance_factor(p->as_dict.left) > 0) {
            p->as_dict.left = rotate_left(p->as_dict.left);
        }

        return rotate_right(p);
    }

    guard_is_one_of(factor, -1, 0, 1);
    return p;
}

size_t object_dict_size(Object *dict) { // NOLINT(*-no-recursion)
    guard_is_not_null(dict);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);
    guard_is_not_null(dict);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return 0;
    }

    return 1 + object_dict_size(dict->as_dict.left) + object_dict_size(dict->as_dict.right);
}

bool object_dict_try_put( // NOLINT(*-no-recursion)
        ObjectAllocator *a,
        Object *dict,
        Object *key,
        Object *value,
        Object **out
) {
    guard_is_not_null(a);
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_not_null(out);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return object_try_make_dict(a, key, value, object_nil(), object_nil(), out);
    }

    auto compare_result = object_compare(key, dict->as_dict.key);

    if (0 == compare_result) {
        return object_try_make_dict(a, key, value, dict->as_dict.left, dict->as_dict.right, out);
    }

    if (compare_result > 0) {
        auto const ok =
                object_try_make_dict(
                        a,
                        dict->as_dict.key,
                        dict->as_dict.value,
                        dict->as_dict.left,
                        dict->as_dict.right,
                        out
                ) && object_dict_try_put(a, dict->as_dict.right, key, value, &(*out)->as_dict.right);
        if (false == ok) {
            return false;
        }

        *out = balance(*out);
        return true;
    }

    auto const ok =
            object_try_make_dict(
                    a,
                    dict->as_dict.key,
                    dict->as_dict.value,
                    dict->as_dict.left,
                    dict->as_dict.right,
                    out
            ) && object_dict_try_put(a, dict->as_dict.left, key, value, &(*out)->as_dict.left);
    if (false == ok) {
        return false;
    }

    *out = balance(*out);
    return true;
}

bool object_dict_try_get(Object *dict, Object *key, Object **value) { // NOLINT(*-no-recursion)
    guard_is_not_null(dict);
    guard_is_not_null(key);
    guard_is_not_null(value);
    guard_is_one_of(dict->type, TYPE_NIL, TYPE_DICT);

    if (object_nil() == dict) {
        return false;
    }

    auto compare_result = object_compare(key, dict->as_dict.key);

    if (0 == compare_result) {
        *value = dict->as_dict.value;
        return true;
    }

    if (compare_result > 0) {
        return object_dict_try_get(dict->as_dict.right, key, value);
    }

    return object_dict_try_get(dict->as_dict.left, key, value);
}
