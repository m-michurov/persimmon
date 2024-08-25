(import "demos/def.scm")

(defmacro -> (init & transforms)
  (defn generate (init transforms)
    (if transforms
      (generate (concat (first transforms) (list init)) (rest transforms))
      init))
  (generate init transforms))