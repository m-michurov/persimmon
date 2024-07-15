(import "demos/def.scm")

(defn map (f col)
  (defn inner (acc col)
    (if col
      (inner (prepend (f (first col)) acc) (rest col))
      (reverse acc)))
  (inner nil col))

(defn apply (f col)
  (if col
    (do
      (f (first col))
      (apply f (rest col)))))

(defn take (n col)
  (defn inner (n acc col)
    (if col
      (if (eq? 0 n)
        (reverse acc)
        (inner (- n 1) (prepend (first col) acc) (rest col)))
      (reverse acc)))
  (inner n nil col))

(defn drop (n col)
  (if col
    (if (eq? 0 n)
      col
      (drop (- n 1) (rest col)))))

(defn take-every (n col)
  (defn inner (acc col)
    (if col
      (inner (prepend (first col) acc) (drop n col))
      (reverse acc)))
  (inner nil col))

(defn chunk-by (n col)
  (defn inner (acc col)
    (if col
      (inner (prepend (take n col) acc) (drop n col))
      (reverse acc)))
  (inner nil col))
