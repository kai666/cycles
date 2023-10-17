# cycles
small tool to measure instruction count of a sub command with linux perf API

# usage

sudo cycles [cmd]

example:
```
$ sudo cycles /bin/ls /
bin   dev  home  lib32	libx32	    media  opt	 root  sbin  srv       sys  usr
boot  etc  lib	 lib64	lost+found  mnt    proc  run   snap  swap.img  tmp  var
cycles: 386851
```
