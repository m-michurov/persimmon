(import "demos/range.lisp")

(print (map (fn (x) (list x (* x x))) (range 15)))
