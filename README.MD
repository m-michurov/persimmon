# Persimmon

Persimmon is a dynamically typed garbage collected 
Lisp-like programming language.

_Work in progress_

## Example code

[demo/fib.pmn](demo/fib.pmn)

```common lisp
(:= fib (fn (n)
    (:= a 0)
    (:= b 1)
    (while (!= 0 n)
        (:= tmp a)
        (= a b)
        (= b (+ tmp b))
        (= n (- n 1)))
    a))

(print "Fibonacci number 7 is" (fib 7))
(print "Fibonacci number 10 is" (fib 10))
```

## Builtins

### Types

 * `int` - a 64 bit signed integer;
 * `string` - an immutable sequence of ASCII characters;
 * `function` - a function;
 * `undefined` - special type with a single value that indicates
an invalid operation.

### Special forms 

 * `(:= <target> <value>)` - variable definition;
 * `(= <target> <value>)` - variable assignment;
 * `(while <condition> <body1> <body> ...)` - `while` loop;
 * `(fn (<arg1> <arg2> ...) <body1> <body> ...)` - function definition.

### Functions

 * `(+ <arg1> <arg2> ...)` - sum ints or concatenate strings;
 * `(- <arg1> <arg2> ...)` - subtract `<arg2>, <arg3>, ...` from `<arg1>`;
 * `(* <arg1> <arg2> ...)` - multiply ints;
 * `(print <arg1> <arg2> ...)` - print values separated by spaces;
 * `(== <arg1> <arg2> ...)` - all arguments are equal;
 * `(!= <arg1> <arg2> ...)` - at least two arguments are not equal.
