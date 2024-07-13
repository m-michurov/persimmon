(import "demos/macros.scm")

(defn reverse (col)
    (let
        ((reverse- (fn (self acc col)
            (if col
                (self self (prepend (first col) acc) (rest col))
                acc))))
        (reverse- reverse- nil col)))

(defn map (f col)
    (let
        ((map- (fn (self acc col)
            (if col
                (self self (prepend (f (first col)) acc) (rest col))
                (reverse acc)))))
        (map- map- nil col)))


(defn apply (f col)
    (if col
        (do
            (f (first col))
            (apply f (rest col)))))

(defn take (n col)
    (let
        ((take- (fn (self n acc col)
            (if col
                (if (eq? 0 n)
                    (reverse acc)
                    (self self (- n 1) (prepend (first col) acc) (rest col)))
                (reverse acc)))))
        (take- take- n nil col)))

(defn drop (n col)
    (if col
        (if (eq? 0 n)
            col
            (drop (- n 1) (rest col)))))

(defn chunk-by (n col)
    (let
        ((chunk-by- (fn (self acc col)
            (if col
                (self self (prepend (take n col) acc) (drop n col))
                (reverse acc)))))
    (chunk-by- chunk-by- nil col)))