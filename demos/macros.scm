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
        (reverse- nil col)))

(defn macros.firsts (col-of-cols)
    (do
        (defn firsts- (acc col)
            (if col
                (do
                    (define (value _) (first col))
                    (firsts- (prepend value acc) (rest col)))
                (macros.reverse acc)))
        (firsts- nil col-of-cols)))

(defn macros.seconds (col-of-cols)
    (do
        (defn seconds- (acc col)
            (if col
                (do
                    (define (_ value) (first col))
                    (seconds- (prepend value acc) (rest col)))
                (macros.reverse acc)))
        (seconds- nil col-of-cols)))

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

