(import "demos/functional.scm")
(import "demos/lists.scm")
(import "demos/let.scm")

(defmacro print-key (key-expr)
  (list 'print (list 'quote key-expr) "->" key-expr))

(define d1 (dict 1 2 "hello" "world"))
(define d2 (put 2 4 d1))
(define d3 (put 3 9 d1))
(define d4 (put d1 d2 d1))

(print d1)
(print d2)
(print d3)
(print d4)

(print '---)

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
                'empty (dict)
                'guards (dict
                          'assert (fn (& _) nil)
                          'unreachable nil
                          'double (fn (x) (* 2 x)))
                'x (dict 'y (dict 'z 42))))
  (print utility)
  (print-key utility.constants)
  (print-key utility.guards)
  (print-key utility.empty)
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
(print (catch (dict).""))
(print (catch (get eq? (dict))))

(print '---)

(let (n 10
       d (reduce
           (fn (d value)
             (-> d
               (put value (* value value))
               (put 'prev d)))
           (dict)
           (range 10)))
  (print d)
  (apply (fn (x) (print (get x d))) (range n))
  (print d.1 d.2 d.3 d.4 d.5)
  (print d))


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