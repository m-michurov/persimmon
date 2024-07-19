(import "demos/lists.scm")
(import "demos/let.scm")

(defn error-get (name fields)
  (if fields
    (let ((binding . rest) fields
           (cur-name value . _) binding)
      (if (eq? name cur-name)
        value
        (error-get name rest)))))

(defn print-traceback (err)
  (let ((_ . fields) err)
    (print "Traceback:")
    (apply print (error-get 'traceback fields))))

(defmacro run-catching (. code)
  (list 'let (list '(val err) (list 'catch (concat '(do) code)))
        '(if err
           (let ((type . fields) err)
             (print 'ERROR type '- (error-get 'message fields))
             (print-traceback err))
           (print 'VALUE val))))

(run-catching (/ 10 0))
(run-catching (/ 10 3))
(run-catching (define x (/ 1 2) (print y)))
(run-catching (define x (/ 1 2)) (print y))
(run-catching (define x (/ 1 2)) x)
(run-catching (catch))
(run-catching (catch 1 2 3))