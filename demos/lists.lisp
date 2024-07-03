(define range (fn (n)
    (define range- (fn (n acc)
        (if (eq? 0 n)
            acc
            (range- (- n 1) (prepend n acc)))))
    (range- n (list))))

(define reverse (fn (col)
    (define reverse- (fn (acc col)
        (if col
            (reverse- (prepend (first col) acc) (rest col))
            acc)))
    (reverse- (list) col)))

(define map (fn (f col)
    (define map- (fn (acc col)
        (if col
            (map- (prepend (f (first col)) acc) (rest col))
            (reverse acc))))
    (map- (list) col)))

(define apply (fn (f col)
    (if col
        (do
            (f (first col))
            (for f (rest col))))))
