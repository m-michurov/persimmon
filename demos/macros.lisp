(define defmacro (macro (name args body)
    (list 'define name (list 'macro args body))))

(defmacro defn (name args body)
    (list 'define name (list 'fn args body)))

(defn macros.reverse (col)
    (do
        (defn reverse- (acc col)
            (if col
                (reverse- (prepend (first col) acc) (rest col))
                acc))
        (reverse- (list) col)))

(defn macros.firsts (col-of-cols)
    (do
        (defn firsts- (acc col)
            (if col
                (firsts- (prepend (first (first col)) acc) (rest col))
                (macros.reverse acc)))
        (firsts- (list) col-of-cols)))

(defn macros.seconds (col-of-cols)
    (do
        (defn seconds- (acc col)
            (if col
                (seconds- (prepend (first (rest (first col))) acc) (rest col))
                (macros.reverse acc)))
        (seconds- (list) col-of-cols)))

(defn macros.concat (a b)
    (do
        (defn concat- (acc col)
            (if col
                (concat- (prepend (first col) acc) (rest col))
                acc))
        (concat- b (macros.reverse a))))

(defmacro let (bindings body)
    (macros.concat
        (list (list 'fn (macros.firsts bindings) body))
        (macros.seconds bindings)))

