(import "demos/lists.scm")
(import "demos/let.scm")
(import "demos/error.scm")

(defn print-traceback (err)
  (print "Traceback:")
  (apply print err.traceback))

(defmacro run-catching (& code)
  (list 'let (list '(val err) (list 'catch (concat '(do) code)))
        '(if err
           (if (dict? err)
             (do
               (print 'ERROR err.type '- err.message)
               (print-traceback err))
             (print 'ERROR err))
           (print 'VALUE val))))

(defn run-ignoring (error-type default runnable)
  (let ((value err) (catch (runnable)))
    (if err
      (if (and (dict? err) (eq? error-type err.type))
        default
        (throw err))
      value)))

(run-catching (/ 10 0))
(run-catching (/ 10 3))
(run-catching (define x (/ 1 2) (print y)))
(run-catching (define x (/ 1 2)) (print y))
(run-catching (define x (/ 1 2)) x)
(run-catching (catch))
(run-catching (catch 1 2 3))

(run-catching (define (x y) '(1 2 3)))
(run-catching (define (x y z t) '(1 2 3)))
(run-catching (define (x y & z t) '(1 2 3)))
(run-catching (define (x y & z) '(1)))
(run-catching (define (x y & 1) '(1 2 3)))
(run-catching (define (x 1) '(1 2 3)))
(run-catching (define (& x) 1))

(run-catching (throw 1))
(run-catching (throw (error 'TypeError '(message "unsupported type") '(expected integer) '(got nil))))
(run-catching (throw (error 1)))
(run-catching (throw (error 1 2 )))
(run-catching (throw (error)))

(run-catching (run-ignoring 'TypeError -1 (fn () (/ 1 0))))
(run-catching (run-ignoring 'TypeError -1 (fn () (/ 1 ""))))
