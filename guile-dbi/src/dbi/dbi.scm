;; dbi.scm - scm glue module code
;; Copyright (C) 2004, 2005, 2026 Free Software Foundation, Inc.
;; Written by Maurizio Boriani <baux@member.fsf.org>
;;
;; This file is part of the guile-dbi.
;;
;; The guile-dbi is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;;
;; The guile-dbi is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
;;
;;                                               -*- Scheme -*-
(define-module (dbi dbi))

(load-extension "libguile-dbi" "init_dbi")

;; exported primitives

(export dbi-open
        dbi-close
        dbi-params-query
        dbi-query
        dbi-get_row
        dbi-get_status)
