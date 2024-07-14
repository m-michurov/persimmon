(define defmacro
  (macro (name args . body)
    (list 'define name (concat (list 'macro args) body))))

(defmacro defn (name args . body)
  (list 'define name (concat (list 'fn args) body)))

(defn _firsts (col-of-cols)
  (defn __firsts (acc col)
    (if col
      (do
        (define (value _) (first col))
        (__firsts (prepend value acc) (rest col)))
      (reverse acc)))
  (__firsts nil col-of-cols))

(defn _seconds (col-of-cols)
  (defn __seconds (acc col)
    (if col
      (do
        (define (_ value) (first col))
        (__seconds (prepend value acc) (rest col)))
      (reverse acc)))
  (__seconds nil col-of-cols))

(defmacro let (bindings body)
  (concat
    (list (list 'fn (_firsts bindings) body))
    (_seconds bindings)))

