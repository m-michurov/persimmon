(import "demos/def.scm")
(import "demos/let.scm")
(import "demos/types.scm")

(defn error (error-type . fields)
  (let (tb (traceback))
    (list
      error-type
      (list 'traceback (rest tb))
      . fields)))
