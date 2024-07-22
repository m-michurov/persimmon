(import "demos/def.scm")
(import "demos/let.scm")
(import "demos/types.scm")
(import "demos/lists.scm")

(defn error (error-type . fields)
  (let (tb (traceback))
    (if (not (atom? error-type))
      (throw (list
               'TypeError
               '(message "error-type must be an atom")
               '(expected atom)
               (list 'got (type error-type))
               (list 'traceback (rest tb)))))
    (list
      error-type
      (list 'traceback (rest tb))
      . fields)))
