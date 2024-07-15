(import "demos/lists.scm")
(import "demos/range.scm")

(defn error-get (name fields)
  (if fields
    (let (((binding . rest) fields))
      (let (((cur-name value . _) binding))
        (if (eq? name cur-name)
          value
          (error-get name rest))))))

(defn print-traceback (err)
  (let (((_ . fields) err))
    (print "Traceback:")
    (apply print (error-get 'traceback fields))))

(defmacro run-catching (. code)
  (let ((result
          (list 'let (list (list '(val err) (list 'try (concat '(do) code))))
                '(if err
                   (let (((type . fields) err))
                     (print 'ERROR type '- (error-get 'message fields))
                     (print-traceback err))
                   (print 'VALUE val)))))
    (print result)
    result))

(run-catching (/ 10 0))
(run-catching (/ 10 3))
(run-catching (define x (/ 1 2) (print y)))
(run-catching (define x (/ 1 2)) (print y))
(run-catching (define x (/ 1 2)) x)
(run-catching (try))
(run-catching (try 1 2 3))
