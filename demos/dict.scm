(import "demos/functional.scm")
(import "demos/lists.scm")
(import "demos/let.scm")

(defmacro print-key (key-expr)
  (list 'print (list 'quote key-expr) "->" key-expr))

(defn dict (& args)
  (let (kw-pairs (chunk-by 2 args))
    (reduce
      (fn (acc (key value)) (put key value acc))
      nil
      kw-pairs)))

(let (d (dict 1 2 "hello" "world" '(1 2 3) (fn (x) x) 'name "rei"))
  (print-key d.1)
  (print-key d."hello")
  (print-key d.(1 2 3))
  (print-key d.name))

(print '---)

(let (utility (dict
                'constants (dict
                             'COMPILER "cc"
                             'VERSION "0.1")
                'guards (dict
                          'assert (fn (& _) nil)
                          'unreachable nil
                          'double (fn (x) (* 2 x)))
                'x (dict 'y (dict 'z 42))))
;  (print-key utility.)
  (print-key utility.constants)
  (print-key utility.guards)
  (print-key utility.constants.COMPILER)
  (print-key utility.constants.VERSION)
  (print-key utility.guards.assert)
  (print-key utility.guards.unreachable)
  (print-key (utility.guards.double 3))
  (print-key utility.x.y.z))

(print '---)

(print (catch 1.x))
(print (catch "".x))
(print (catch nil.x))
(print (catch ().x))

(print '---)

(define d (reduce
            (fn (dict value) (put value (* value value) dict))
            nil
            (range 5)))
(print d)
(apply (fn (x) (print (get x d))) (range 5))
(print d.1 d.2 d.3 d.4 d.5)

(print '---)

(let (table (dict
              '(true true) true
              '(true false) false
              '(false true) false
              '(false false) false))
  (print-key table.(true true))
  (print-key table.(true false))
  (print-key table.(false true))
  (print-key table.(false false)))