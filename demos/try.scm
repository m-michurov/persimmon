(import "demos/lists.scm")
(import "demos/range.scm")

(let (((val err) (try (/ 10 0))))
  (if err
    (print (first err))
    (print val)))

(let (((val err) (try (/ 10 4))))
  (if err
    (print (first err))
    (print val)))
