(import "demos/lists.lisp")

(print (map (fn (x) (list x (* x x))) (range 15)))
