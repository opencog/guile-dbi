#!/usr/bin/env guile
-s
!#

(use-modules (dbi dbi))

(define (check headline result expected) (display ";;; ")
  (display headline)
  (if (equal? result expected) (begin (display " PASSED") (newline))
    (begin (display " FAILED: ") (display (list "equal?" result expected)) (newline))))

(define db-path
  (let* ((template (string-copy "/tmp/guile-dbi-test-XXXXXX"))
         (port (mkstemp! template)))
    (close-port port)
    template))
(define db-obj (dbi-open "sqlite3" db-path))

(check "create table"
  (begin (dbi-query db-obj "create table testtable(id int, name varchar(15))")
    (dbi-get_status db-obj))
  '(0 . "query ok"))

(check "insert"
  (begin (dbi-query db-obj "insert into testtable ('id', 'name') values('33', 'testname1')")
    (dbi-query db-obj "insert into testtable ('id', 'name') values('34', 'testname1')")
    (dbi-query db-obj "insert into testtable ('id', 'name') values('44', 'testname2')")
    (dbi-get_status db-obj))
  '(0 . "query ok"))

(check "select"
  (begin (dbi-query db-obj "select * from testtable where name='testname1'")
    (dbi-get_status db-obj))
  '(0 . "query ok"))

(check "get row" (dbi-get_row db-obj) '(("id" . 33) ("name" . "testname1")))

(check "select non-existing row"
  (begin (dbi-query db-obj "select * from testtable where name='testname'") (dbi-get_status db-obj))
  '(0 . "query ok"))

(check "get non-existing row" (dbi-get_row db-obj) #f)

(check "count" (begin (dbi-query db-obj "select count(id) from testtable") (dbi-get_status db-obj))
  '(0 . "query ok"))

(check "get count" (dbi-get_row db-obj) '(("count(id)" . 3)))
(dbi-close db-obj)

(check "open/close not leaking file descriptors, not segfaulting"
  (do ((i 0 (1+ i)) (result #f)) ((> i 2048) result)
    (let ((db-obj (dbi-open "sqlite3" db-path))) (dbi-query db-obj "SELECT 42")
      (dbi-query db-obj "BEGIN IMMEDIATE TRANSACTION")
      (dbi-query db-obj "CREATE TABLE IF NOT EXISTS testtable (id INTEGER)")
      (dbi-query db-obj "INSERT INTO testtable SET (id) VALUES (23)")
      (dbi-query db-obj "SELECT * FROM testtable") (dbi-query db-obj "SELECT * FROM testtable")
      (dbi-query db-obj "SELECT * FROM non-existent") (dbi-query db-obj "COMMIT TRANSACTION")
      (when (= i 1024) (set! result (dbi-get_status db-obj))) (dbi-close db-obj)))
  '(0 . "query ok"))

(delete-file db-path)
