(define range (fn (n)
    (define range- (fn (n acc)
        (if (eq? 0 n)
            acc
            (range- (- n 1) (prepend n acc)))))
    (range- n (list))))
