(import "demos/compare.scm")
(import "demos/let.scm")

(print (compare 1 1))
(print (compare 1 2))
(print (compare 1 -2))
(print (compare "1" -2))

(let (l1 (list 1 2 3 4)
       l2 (list 1 2 3 4))
  (print l1)
  (print l2)
  (print (compare l1 l2)))

(let (l1 (list 1 2 3)
       l2 (list 1 2 3 4))
  (print l1)
  (print l2)
  (print (compare l1 l2)))

(let (l1 (list 1 2 3)
       l2 (list 1 2))
  (print l1)
  (print l2)
  (print (compare l1 l2)))

(let (l1 (list 1 2 4 4)
       l2 (list 1 2 3 4))
  (print l1)
  (print l2)
  (print (compare l1 l2)))

(let (l1 (list 1 2 4 4 1)
       l2 (list 1 2 5 4))
  (print l1)
  (print l2)
  (print (compare l1 l2)))

(let (d1 (dict 1 2 3 4)
       d2 (dict 1 2 3 4))
  (print d1)
  (print d2)
  (print (compare d1 d2)))

(let (d1 (dict 1 2 "3" 4)
       d2 (dict 1 2 3 4))
  (print d1)
  (print d2)
  (print (compare d1 d2)))

(let (d1 (dict 1 2 3 4)
       d2 (dict 1 4 3 8))
  (print d1)
  (print d2)
  (print (compare d1 d2)))

(let (d1 (dict 1 4 3 8)
       d2 (dict 1 2 3 4))
  (print d1)
  (print d2)
  (print (compare d1 d2)))

(let (d1 (dict 1 2 3 3)
       d2 (dict 1 2 2 3))
  (print d1)
  (print d2)
  (print (compare d1 d2)))

(let (d1 (dict 1 2 2 3)
       d2 (dict 1 2 3 3))
  (print d1)
  (print d2)
  (print (compare d1 d2)))