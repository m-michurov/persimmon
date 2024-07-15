(import "demos/lists.scm")
(import "demos/range.scm")

(print (try (/ 10 0)))
(print (try (/ 10 4)))

(defn print-traceback (err)
  (let (((_ . rest) err))
    (apply (fn ((name value . _))
             (if (eq? 'traceback name)
               (do
                 (print "Traceback:")
                 (apply print value))))
      rest)))

(defmacro run-catching (. code)
  (let ((result
          (list 'let (list (list '(val err) (list 'try (concat '(do) code))))
                '(if err
                   (let ((type . _) err)
                     (print 'ERROR type)
                     (print-traceback err))
                   (print 'VALUE val)))))
    (print result)
    result))

(run-catching (/ 10 0))
(run-catching (/ 10 3))
(run-catching (define x (/ 1 2) (print y)))
