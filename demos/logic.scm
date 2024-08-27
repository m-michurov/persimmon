(print "-- eq? -- ")

(print (eq? nil nil))
(print (eq? '(1 2 3) nil))
(print (eq? '(1 2 3) '(1 2 3)))
(print (eq? '(1 2 3) '(1 2 3 4)))
(print (eq? '(1 2 3) (list 1 2 3)))
(print (eq? '(1 2 (a b)) (list 1 2 (list 'a 'b))))

(print "-- and -- ")

(print (and true true))
(print (and true false))
(print (and false (do (print 'a) true)))
(print (and false false))

(print "-- or -- ")

(print (or true true))
(print (or true (do (print 'a) false)))
(print (or false true))
(print (or false false))

(print "-- not -- ")

(print (not true))
(print (not false))

(print (or nil 42))
(print (or "str" "default"))
(print (or nil "default" nil))
