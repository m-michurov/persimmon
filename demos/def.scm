(define defmacro
  (macro (name args . body)
    (list 'define name (concat (list 'macro args) body))))

(defmacro defn (name args . body)
  (list 'define name (concat (list 'fn args) body)))
