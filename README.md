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

```
$ sudo ./cycles -u 100 -g "100,1000,3000" /bin/id
uid=100(messagebus) gid=100(users) groups=100(users),1000(kai),3000(azure)
cycles: 490215
```

```
$ sudo ./cycles -u kai -g kai /bin/id
uid=1000(kai) gid=1000(kai) groups=1000(kai)
cycles: 460964
```
