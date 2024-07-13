(import "demos/lists.scm")
(import "demos/range.scm")

(apply print (chunk-by 6 (map (fn (x) (list x (* x x))) (range 500))))
