# Persimmon

Persimmon is a functional programming language inspired by Scheme, Clojure, Lua and Python.

Persimmon is not and probably will never be a feature-complete language you can use 
for anything remotely serious. Still, working on Persimmon helped me explore various 
CS concepts and topics.

Below is the (probably incomplete) list of resources I used while creating Persimmon in no particular order.

 * [MiniLisp by Rui Ueyama (GitHub)](https://github.com/rui314/minilisp/tree/master)
 * Structure and Interpretation of Computer Programs by Harold Abelson, Gerald Jay Sussman and Julie Sussman ([Wikipedia](https://en.wikipedia.org/wiki/Structure_and_Interpretation_of_Computer_Programs))
 * [Crafting Interpreters by Robert Nystrom](https://craftinginterpreters.com/)
 * [ClojureDocs](https://clojuredocs.org/)

## Code Example

*From [main.scm](demos/main.scm):*

```scheme
(import "demos/lists.scm")
(import "demos/let.scm")
(import "demos/functional.scm")

(defn main () ; comment
  (let (square (fn (x) (* x x))
         square-pair (fn (x) (list x (square x)))
         format (fn ((num squared)) (str num " squared is " squared)))
    (-> 500
      (range)
      (map square-pair)
      (map format)
      (chunk-by 3)
      (apply print))))

(main)
```

## Build

Persimmon is built using [CMakeLists.txt](CMakeLists.txt).

## Run

Run REPL:

```
$> persimmon
```

Run file `SOURCE`:

```
$> persimmon SOURCE
```

## Memory management

Persimmon uses mark-and-sweep garbage collection.

## Recursion

Persimmon applies tail call optimisation whenever possible.

## Data types

Following types are available:

 * `integer` - 64-bit signed integer.
 * `string` - an immutable ASCII string.
 * `symbol` - an immutable sequence of printable non-whitespace characters.
 * `cons` - an immutable singly linked list.
 * `dict` - a persistent immutable mapping.
 * `primitive` - a native function implemented as part of the interpreter.
 * `closure` - a closure.
 * `macro` - a closure that returns code instead of a value.
 * `nil` - nil value, an empty list.

There is no boolean type; `nil` is treated as false and everything else is true.

**Code is data.** Literals are just objects of corresponding types and symbolic expressions
are `cons` lists.

## Call syntax

`(f a b c)` - apply `f` to `(a b c)`.

`(f a b & rest)` - apply `f` to concatenation of `(a b)` and `rest`, 
for example `(+ 1 2 & '(3 4 5))` is equivalent to `(+ 1 2 3 4 5)`.

## Structured binding

A binding target can be an `symbol` or a list of valid binding targets, possibly 
ending with `& rest` where `rest` is a name that will capture the excess elements, if any.

Examples:

```scheme
>>> (define x 1)
1
>>> x
1
>>> (define (x y) '(3 4))
(3 4)
>>> x
3
>>> y
4
>>> (define (x (y z & _) t & rest) '(1 (2 3 4 5) 6 7 8 9))
(1 (2 3 4 5) 6 7 8 9)
>>> x y z t rest
1
2
3
6
(7 8 9)
```

## Error handling

Persimmon uses exceptions. 
Built-in special forms and primitives throw tagged lists as errors.
Such errors typically look like:

```scheme
(error-type
  (message "...")
  (field1 value1)
  ...
  (traceback ...))
```

This form is recognized by the interpreter and is reported in a human-readable format.
For example, if the following error is not handled (formatted for readability):

```scheme
(KeyError 
  (message "key does not exist: 2") 
  (key 2) 
  (traceback  ((get '2 d) 
               (catch (get '2 d)))))
```

it is reported as:
```
KeyError: key does not exist: 2
Traceback (most recent call last):
    (get '2 d)
Some calls may be missing due to tail call optimization.
```

See `throw` primitive and `catch` special form.

## Special forms

 * `(if cond then)` - return the value of `then` if `cond` is evaluated 
to non-`nil` value, `nil` otherwise.
 * `(if cond then else)` - same, but returns the value of `else` if
`cond` is evaluated to `nil`.
 * `(do & exprs)` - evaluate arguments in order, the resulting value is
the value of the last expression.
 * `(define target value)` - bind `target` to `value` in the current scope.
 * `(fn args & body)` - create a closure, `args` must be a valid binding target.
 * `(macro args & body)` - create a macro, `args` must be a list of `symbol`s.
 * `(import path)` - read and execute file `path` in the current scope, `path` must be
a string; top-level expressions are executed in an implicit `do`.
 * `'expr` or `(quote expr)` - evaluates to `expr`.
 * `(catch expr)` - if `expr` is evaluated to `value`, `(catch expr)` evaluates to
`(value nil)`; if `expr` throws an error `error`, `(catch expr)` evaluates to
`(nil error)`.
 * `(and & args)` - evaluates arguments in order; returns `nil` as soon as one of the
  arguments is evaluated to `nil` and the last argument's value otherwise.
 * `(or & args)` - evaluates arguments in order; returns first non-`nil` value.

## Constants and primitives

#### Constants

 * `nil`.
 * `true` - evaluates to symbol `true`.
 * `false` - alias for `nil`.

#### Primitives

 * `(eq? a b)` - returns `'true` if `a` is equal to `b`.
 * `(str & strs)` - stringify and concatenate arguments.

Example:
```scheme
>>> (str 1 " + " 2 " is " (+ 1 2)) 
"1 + 2 is 3"
```
 * `(repr it)` - returns a string representation of `it`.
 * `(print & args)` - stringify and print arguments separated by a single space, add 
newline after the last argument.
 * `(+ & args)` - sum numbers.
 * `(- & args)` - subtract numbers.
 * `(* & args)` - multiply numbers.
 * `(/ & args)` - divide numbers.
 * `(list & args)` - returns `args`, i.e. a list that contains provided arguments.
 * `(first list)` - returns the head of the list `list`.
 * `(first list)` - returns the tail of the list `list`.
 * `(prepend element list)` - returns a list with `element` as head and `list` as tail.
 * `(reverse list)` - returns a reversed copy of `list`.
 * `(concat & lists)` - returns a concatenation of arguments.
 * `(dict & args)` - constructs a dict with given keys and values; takes an even number of arguments.

Example:

```scheme
>>> (dict 1 2 3 4 "hello" "world")
{1 2, 3 4, "hello" "world"}
```

 * `(get key dict)` - returns a value at given `key` or throws a `KeyError`.

The `.` (dot) operator can be used if keys are known literals.

Example:
```scheme
>>> (define d (dict 1 2 'key 'value "hello" "world" (list 2 4) (list 6 8))) 
{1 2, key value, "hello" "world", (2 4) (6 8)}
>>> d.1
2
>>> d.key
value
>>> d."hello"
"world"
>>> d.(2 4)  
(6 8)
>>> d.2
KeyError: key does not exist: 2
Traceback (most recent call last):
    (get '2 d)
Some calls may be missing due to tail call optimization.
```

 * `(put key value dict)` - returns a new dict with `key` mapped to `value`; old 
dict is unchanged.
 * `(not it)` - returns `'true` if `it` is `nil` and `nil` otherwise.
 * `(type it)` - returns the name of the type of `it` as an symbol.
 * `(traceback)` - returns the current expression stack as a list, most recent call comes last.
 * `(throw error)` - throw `error`; `error` can be any value other than `nil`


