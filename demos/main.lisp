(import "demos/lists.lisp")
(import "demos/range.lisp")

(apply (fn (x) (/ 1 0)) (chunk-by 13 (map (fn (x) (list x (* x x))) (range 100))))
