The MySQL DBD driver
--------------------

This is the driver used to connect to MySQL database servers.

Connection string format:
```
 <user>:<pass>:<db name>:<connection type>:<path or ip>:[port]:[recon attempts]
```
where
* `<connection type>` must be one of the two strings "tcp" or "socket"
* `<path or ip>` specify the path of the socket or the ip address of the server
* `<port>` is used only if the connection type is "tcp"
* `<recon attempts>`: optional. Number of reconnection attempts after a db
   connection failure. Default, if omitted, is 0 (no reconnection will be
   attempted).
