(import "demos/def.scm")

(defmacro print-truth (expr)
  (list 'print (list 'quote expr) ''-> expr))

(print "-- eq? -- ")

(print-truth (eq? nil nil))
(print-truth (eq? '(1 2 3) nil))
(print-truth (eq? '(1 2 3) '(1 2 3)))
(print-truth (eq? '(1 2 3) '(1 2 3 4)))
(print-truth (eq? '(1 2 3) (list 1 2 3)))
(print-truth (eq? '(1 2 (a b)) (list 1 2 (list 'a 'b))))
(print-truth (eq? (dict 1 2 3 4) (dict 1 4 3 9)))
(print-truth (eq? (dict 1 2 3 4) (dict 3 4 1 2)))

(print "-- and -- ")

(print-truth (and true true))
(print-truth (and true false))
(print-truth (and false (do (print 'a) true)))
(print-truth (and false false))

(print "-- or -- ")

(print-truth (or true true))
(print-truth (or true (do (print 'a) false)))
(print-truth (or false true))
(print-truth (or false false))

(print "-- not -- ")

(print-truth (not true))
(print-truth (not false))

(print-truth (or nil 42))
(print-truth (or "str" "default"))
(print-truth (or nil "default" nil))
