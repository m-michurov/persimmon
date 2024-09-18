(import "demos/def.scm")
(import "demos/let.scm")
(import "demos/types.scm")
(import "demos/lists.scm")

(defn error (error-type message)
  (let (tb (traceback))
    (if (not (symbol? error-type))
      (throw (error 'TypeError "error-type must be a symbol")))
    (dict
      'type error-type
      'message message
      'traceback (traceback))))
