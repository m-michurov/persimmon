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
            (apply f (rest col))))))

(define take (fn (n col)
    (define take- (fn (n acc col)
        (if col
            (if (eq? 0 n)
                (reverse acc)
                (take- (- n 1) (prepend (first col) acc) (rest col)))
            (reverse acc))))
    (take- n (list) col)))

(define drop (fn (n col)
    (if col
        (if (eq? 0 n)
            col
            (drop (- n 1) (rest col)))
        (list))))

(define chunk-by (fn (n col)
    (define chunk-by- (fn (acc col)
        (if col
            (chunk-by- (prepend (take n col) acc) (drop n col))
            (reverse acc))))
    (chunk-by- (list) col)))
