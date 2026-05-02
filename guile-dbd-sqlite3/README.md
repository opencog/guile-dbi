The Sqlite3 DBD driver
----------------------

This a driver for `guile-dbi` to use a `sqlite3` database.

This version was created by copying the original implementation at
https://github.com/jkalbhenn/guile-dbd-sqlite3

Connection string format:
```
  <database name>
```

Run tests individually as
```
	guile test-guile-dbd-sqlite3.scm
```
or all together as
```
make check
```
