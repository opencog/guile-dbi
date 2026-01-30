#!/usr/bin/env guile
-s
!#
;;; test-guile-dbi.scm - Unit tests for guile-dbi using SRFI-64
;;;
;;; Copyright (C) 2004, 2005 Free Software Foundation, Inc.
;;; Written by Maurizio Boriani <baux@member.fsf.org>
;;; Ported to SRFI-64 in 2026.
;;;
;;; This file is part of the guile-dbi.
;;;
;;; The guile-dbi is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2 of the License, or
;;; (at your option) any later version.
;;;
;;; The guile-dbi is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <https://www.gnu.org/licenses/>.

(use-modules (srfi srfi-64)
             (dbi dbi))

(define dbh #f)

(test-begin "guile-dbi")

(test-assert "create a guile dbi handler object"
  (begin
    (set! dbh (dbi-open "test" "test"))
    (zero? (car (dbi-get_status dbh)))))

(test-assert "guile dbi query"
  (begin
    (dbi-query dbh "select * from test;")
    (zero? (car (dbi-get_status dbh)))))

(test-assert "guile dbi get row"
  (let ((row (dbi-get_row dbh)))
    (zero? (car (dbi-get_status dbh)))))

(test-assert "guile dbi close connection"
  (begin
    (dbi-close dbh)
    (zero? (car (dbi-get_status dbh)))))

(test-assert "reopen a closed connection"
  (begin
    (set! dbh (dbi-open "test" "test"))
    (zero? (car (dbi-get_status dbh)))))

(test-assert "guile dbi query after reopen"
  (begin
    (dbi-query dbh "select * from test;")
    (zero? (car (dbi-get_status dbh)))))

(test-assert "guile dbi close a re-opened connection"
  (begin
    (dbi-close dbh)
    (zero? (car (dbi-get_status dbh)))))

;; These tests expect failure (non-zero status)
(test-assert "guile dbi query using closed connection fails"
  (begin
    (dbi-query dbh "select * from test;")
    (not (zero? (car (dbi-get_status dbh))))))

(test-assert "guile dbi get row using closed connection fails"
  (let ((row (dbi-get_row dbh)))
    (not (zero? (car (dbi-get_status dbh))))))

(test-assert "close a closed connection is no-op"
  (begin
    (dbi-close dbh)
    ;; Closing an already-closed connection should not error
    #t))

(test-end "guile-dbi")

;; Exit with proper status for make check
(exit (if (zero? (test-runner-fail-count (test-runner-current))) 0 1))
