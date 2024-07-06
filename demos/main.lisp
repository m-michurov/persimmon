(import "demos/lists.lisp")

(print (map (fn (x) (list x "squared" (* x x))) (range 100)))
