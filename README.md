# Welcome to mohican

## What's mohican?
Mohican is a new web-server made for UNIX systems.

What it can do:
* Mohican can process http requests.
* Mohican can connect to other upstreams.
* Mohican uses config file.
* Mohican is controlled by command line.
* Write information into logs. 

## Commands to control mohican server
### Start mohican server
```
$ sudo ./mohican.sh start
```

### Create new config
```
$ sudo ./mohican.sh create config
```

### Get status
```
$ sudo ./mohican.sh status
```

### Reload mohican server with new changes in config file
```
$ sudo ./mohican.sh reload
```

### Stop hard mohican server
```
$ sudo ./mohican.sh stop hard
```

### Stop soft mohican server
```
$ sudo ./mohican.sh stop soft
```
