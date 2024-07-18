(import "demos/def.scm")

(defn log-info (. args)
  (print 'INFO . args))

(defn log-warning (. args)
  (print 'WARNING . args))

(defn log-error (. args)
  (print 'ERROR . args))

(define address "192.168.0.1")
(define timeout-limit 6)

(log-info "Connection established with" address)
(log-warning "Timeout exceeded" timeout-limit "times")
(log-error "Connection interrupted")
