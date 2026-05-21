#!/usr/bin/env guile
-s
!#

#|
* Copyright (C) 2004, 2005, 2006, 2026 Free Software Foundation, Inc.
* Written by Maurizio Boriani <baux@member.fsf.org>
*
* This file is part of the guile-dbi.
*
* The guile-dbi is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* The guile-dbi is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
|#

;;; test-guile-dbi-sqlite.scm - Unit tests for guile-dbi with SQLite3

(use-modules (dbi dbi)
             (srfi srfi-1))

;; Helper functions
(define (check headline result expected)
  (display ";;; ")
  (display headline)
  (if (equal? result expected)
      (begin (display " PASSED") (newline))
      (begin (display " FAILED: ") (display (list "expected" expected "got" result)) (newline))))

(define (check-status headline db-obj)
  (let ((status (dbi-get_status db-obj)))
    (display ";;; ")
    (display headline)
    (if (zero? (car status))
        (begin (display " PASSED") (newline) #t)
        (begin (display " FAILED: ") (display status) (newline) #f))))

;; Temporary SQLite file

(define db-path
  (string-append "/tmp/guile-dbi-test-" (symbol->string (gensym)) ".sqlite"))

(display ";;; Using SQLite database: ")
(display db-path)
(newline)

;; Open connection

(define db-obj (dbi-open "sqlite3" db-path))

(check-status "connect to SQLite" db-obj)

;; Drop table if exists
(dbi-query db-obj "DROP TABLE IF EXISTS dbi_test_table")

;; Create table
(check-status "create table"
  (begin
    (dbi-query db-obj "CREATE TABLE dbi_test_table (id INTEGER, name VARCHAR(50))")
    db-obj))

;; Insert rows
(check-status "insert row 1"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (33, 'testname1')") db-obj))
(check-status "insert row 2"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (34, 'testname1')") db-obj))
(check-status "insert row 3"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (44, 'testname2')") db-obj))

;; Select rows
(check-status "select rows where name='testname1'"
  (begin (dbi-query db-obj "SELECT * FROM dbi_test_table WHERE name='testname1' ORDER BY id") db-obj))

;; Get rows
(define row1 (dbi-get_row db-obj))
(check "get first row" row1 '(("id" . 33) ("name" . "testname1")))
(define row2 (dbi-get_row db-obj))
(check "get second row" row2 '(("id" . 34) ("name" . "testname1")))

;; Select non-existing row
(dbi-query db-obj "SELECT * FROM dbi_test_table WHERE name='nonexistent'")

(check "get non-existing row" (dbi-get_row db-obj) #f)

;; Count query
(check-status "count query"
  (begin (dbi-query db-obj "SELECT COUNT(id) as cnt FROM dbi_test_table") db-obj))
(define count-row (dbi-get_row db-obj))
(check "get count" (cdr (assoc "cnt" count-row)) 3)

;;; Affected rows tests

;; Test 1: single insert
(check-status "insert single row for affected_rows"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (1, 'Alice')") db-obj))
(define affected-after-single-insert (dbi-affected-rows db-obj))
(if (not (= affected-after-single-insert 1))
    (begin
      (display ";;; affected_rows after single insert FAILED: ")
      (display "(expected 1 got ")
      (display affected-after-single-insert)
      (display ")")
      (newline))
   (begin
      (display ";;; affected_rows after single insert PASSED")
      (newline)))

;; Test 2: multiple insert
(check-status "insert multiple rows for affected_rows"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (2, 'Bob'), (3, 'Carol'), (4, 'Dave')") db-obj))
(define affected-after-multi-insert (dbi-affected-rows db-obj))
(if (not (= affected-after-multi-insert 3))
    (begin
      (display ";;; affected_rows after multiple inserts FAILED: ")
      (display "(expected 3 got ")
      (display affected-after-multi-insert)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after multiple inserts PASSED")
      (newline)))

;; Test 3: update rows
(check-status "update rows for affected_rows"
  (begin (dbi-query db-obj "UPDATE dbi_test_table SET name='Updated' WHERE id IN (2,3)") db-obj))
(define affected-after-update (dbi-affected-rows db-obj))
(if (not (= affected-after-update 2))
    (begin
      (display ";;; affected_rows after update FAILED: ")
      (display "(expected 2 got ")
      (display affected-after-update)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after update PASSED")
      (newline)))

; Test 4: update no match
(check-status "update no matching rows for affected_rows"
  (begin (dbi-query db-obj "UPDATE dbi_test_table SET name='None' WHERE id=999") db-obj))
(define affected-after-no-match (dbi-affected-rows db-obj))
(if (not (= affected-after-no-match 0))
    (begin
      (display ";;; affected_rows when update matches nothing FAILED: ")
      (display "(expected 0 got ")
      (display affected-after-no-match)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows when update matches nothing PASSED")
      (newline)))

;; Test 5: delete
(check-status "delete row for affected_rows"
  (begin (dbi-query db-obj "DELETE FROM dbi_test_table WHERE id=1") db-obj))
(define affected-after-delete (dbi-affected-rows db-obj))
(if (not (= affected-after-delete 1))
   (begin
      (display ";;; affected_rows after delete FAILED: ")
      (display "(expected 1 got ")
      (display affected-after-delete)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after delete PASSED")
      (newline)))

;; Test 6: affected_rows after select (sqlite3_changes() returns 0 for SELECT)
(dbi-query db-obj "DROP TABLE IF EXISTS dbi_test_table")
(dbi-query db-obj "CREATE TABLE dbi_test_table (id INTEGER, name VARCHAR(50))")
(check-status "select for affected_rows"
  (begin (dbi-query db-obj "SELECT * FROM dbi_test_table") db-obj))
(define affected-after-select (dbi-affected-rows db-obj))
(if (not (= affected-after-select 0))
    (begin
      (display ";;; affected_rows after select FAILED: ")
      (display "(expected 0 got ")
      (display affected-after-select)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows after select PASSED")
      (newline)))

;; Test 7: sequence of inserts
(check-status "first insert for sequence test"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (10, 'Test1')") db-obj))
(define first-affected (dbi-affected-rows db-obj))

(check-status "second insert for sequence test"
  (begin (dbi-query db-obj "INSERT INTO dbi_test_table (id, name) VALUES (11, 'Test2'), (12, 'Test3')") db-obj))
(define second-affected (dbi-affected-rows db-obj))

(if (not (and (= first-affected 1) (= second-affected 2)))
    (begin
      (display ";;; affected_rows updates after each query FAILED: ")
      (display "(expected first=1 second=2, got first=")
      (display first-affected)
      (display " second=")
      (display second-affected)
      (display ")")
      (newline))
    (begin
      (display ";;; affected_rows updates after each query PASSED")
      (newline)))

;; Cleanup
(dbi-query db-obj "DROP TABLE dbi_test_table")
(dbi-close db-obj)
(delete-file db-path)

(display ";;; All SQLite tests completed")
(newline)
