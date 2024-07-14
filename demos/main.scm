(import "demos/lists.scm")
(import "demos/range.scm")

(defn main ()
  (let ((squares (map (fn (x) (list x (* x x))) (range 500))))
    (apply print (chunk-by 3 (map (fn ((num squared)) (list num 'squared 'is squared)) squares)))))

(main)
