(import "demos/lists.scm")
(import "demos/let.scm")
(import "demos/functional.scm")

(defn main ()
  (let (square (fn (x) (* x x))
         square-pair (fn (x) (list x (square x)))
         format (fn ((num squared)) (list num 'squared 'is squared)))
    (-> 500
      (range)
      (map square-pair)
      (map format)
      (chunk-by 3)
      (apply print))))

(main)
