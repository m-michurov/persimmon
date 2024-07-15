(import "demos/def.scm")
(import "demos/let.scm")

(defn range (n)
  (let (range- (fn (self n acc)
                 (if (eq? 0 n)
                   acc
                   (self self (- n 1) (prepend n acc)))))
    (range- range- n nil)))
