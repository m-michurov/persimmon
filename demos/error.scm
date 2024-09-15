(import "demos/def.scm")
(import "demos/let.scm")
(import "demos/types.scm")
(import "demos/lists.scm")

(defn error (error-type & fields)
  (let (tb (traceback))
    (if (not (symbol? error-type))
      (throw (list
               'TypeError
               '(message "error-type must be a symbol")
               '(expected symbol)
               (list 'got (type error-type))
               (list 'traceback (rest tb)))))
    (list
      error-type
      (list 'traceback (rest tb))
      & fields)))
