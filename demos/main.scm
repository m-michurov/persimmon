(import "demos/lists.scm")
(import "demos/range.scm")

(defn main ()
  (let (data (range 500)
         squares (map (fn (x) (list x (* x x))) data)
         formatted (map (fn ((num squared)) (list num 'squared 'is squared)) squares)
         chunked (chunk-by 3 formatted))
    (apply print chunked)))

(main)
