(import "demos/lists.lisp")
(import "demos/range.lisp")

(apply print (chunk-by 13 (map (fn (x) (list x (* x x))) (range 100))))
