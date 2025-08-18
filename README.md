Guile DBI
=========

**guile-dbi** provides a simple, generic, easy-to-use **guile scheme**
interface to SQL databases, such as Postgres, MySQL or SQLite3.

The system is 'generic' in the sense that the same programming interface
(front end) can be used with different databases. The DBI (database
independent) part provides the scheme interfaces. The DBD (database
dependent) plugins connect to an actual SQL server. Currently, there are
DBD back-ends for Postgres, MySQL and SQLite3. Creating additional DBD
back-ends requires a small amount of C coding, but is a straightforward
task.

Guile-dbi is simple - which is both a blessing and a curse. For the most
part, all that it does is to accept guile strings encoding SQL
statements, forward these to the database, and return rows as scheme
association lists. A minimal amount of translation is performed - for
example, SQL floating point columns are converted to scheme floating
point numbers - but there is nothing fancier than this - there is no
special treatment for dates, currencies, etc. At this time, there is no
support for prepared statements. Capable programmers are invited to add
support for this and other missing features.

Someone clever might want to figure out how to replace the C code by
generic FFI interfaces, so that there would not be any need at all for
any C code. However, that takes more work, and the code here works OK,
so, for now, things seem OK.

Documentation
-------------
The guile-dbi user-manual and reference is
[here](https://htmlpreview.github.io/?https://github.com/opencog/guile-dbi/blob/master/website/guile-dbi.html).

A copy of the old, defunct website is
[here](https://htmlpreview.github.io/?https://github.com/opencog/guile-dbi/blob/master/website/index.html).

Mailing List
------------
All discussion of guile-dbi should be directed to: guile-user@gnu.org.

Bugs should be reported to the
[guile-dbi github site](https://github.com/opencog/guile-dbi).

Building and Installing
----------------------
Currently, the source code is organized into four distinct projects:
The main database-independent code in DBI, and three different DBD
drivers.  You have to build each independently.  You will need to do
something like this:

```
apt-get install autoconf automake texinfo gettext
```
or maybe
```
yum install autoconf automake texinfo gettext
```
Some users might also need to install `autoconf-archive`.

Next:
```
cd guile-dbi
./autogen.sh --no-configure
```
If the above has errors, it is possible that some autoconf macros
cannot be found. In that case, try the below, adjusting `ACLOCAL_PATH`
as needed.
```
ACLOCAL_PATH=/usr/local/share/aclocal/ ./autogen.sh --no-configure
```
Next:
```
mkdir build
cd build
../configure
make
sudo make install
```
Then
```
cd guile-dbd-[mysql, postgresql, sqlite3]
./autogen.sh --no-configure
mkdir build
cd build
../configure
make
sudo make install
```

After doing this, the tutorial in
[the user manual](https://htmlpreview.github.io/?https://github.com/opencog/guile-dbi/blob/master/website/guile-dbi.html)
should work fine.


License
-------
Guile-dbi is distributed under the Gnu GPLv2 license. Code and website
were developed by Maurizio Boriani (2005-2006) and are currently
maintained by Linas Vepstas (2008-2019).
