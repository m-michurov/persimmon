(define print-list (fn (x) (print (first x)) (if x (print-list (rest x)))))(print-list (list 1 2 3 4 5 6 7 8 9 10
11 12 13 14 15 16 17 18 19 20 21 22 23 24 25))

;(reduce
;    (lambda (x y) (+ x y))
;    (prim_list_list "hello\n", "world\\2", "wor\t\"" 1 2 3-4 -69