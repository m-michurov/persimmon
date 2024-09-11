(import "demos/def.scm")

(defn < (a b)
  (eq? -1 (comapre a b)))

(defn <= (a b)
  (not (eq? 1 (comapre a b))))

(defn > (a b)
  (eq? 1 (comapre a b)))

(defn >= (a b)
  (not (eq? -1 (comapre a b))))