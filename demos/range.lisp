(import "demos/macros.lisp")

(defn range (n)
    (let
        ((range- (fn (self n acc)
            (if (eq? 0 n)
                acc
                (self self (- n 1) (prepend n acc))))))
        (range- range- n (list))))
