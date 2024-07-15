(import "demos/def.scm")
(import "demos/lists.scm")

(defmacro let (bindings . body)
  (defn generate (bindings body)
    (if bindings
      (do
        (define (name init) (first bindings))
        (list
          (list 'fn (list name)
                (generate (rest bindings) body))
          init))
      (list (concat (list 'fn '()) body))))
  (generate (chunk-by 2 bindings) body))
