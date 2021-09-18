# ftpotd

This is the program source code for my final year dissertation. The idea was to create a (very basic) implementation
of the FTP protocol, alongside an in memory virtual FS, in order to lure hackers in. 

The FS would be read only and contain a number of enticing files which hackers would be drawn to. All commands and attempted
logins to the system would be logged to a MySQL server for research purposes.

The networking side would consist of fully asynchronous networking sockets, allowing numerous clients to interact simultaneously
with the service.

I'm releasing this with the hope of giving some inspiration/educational reading to anyone who stumbles across this project.

The code is by no means professional, and is bound to have bugs/exploitable code within it. 
In other words: DO NOT RUN THIS ON A LIVE SYSTEM WITH PERSONAL/PRIVATE DATA.

- snoom

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

What things you need to install.

*c++11
*libevent
```
	wget http://monkey.org/~provos/libevent-1.4.14b-stable.tar.gz
	tar xzf libevent-1.4.14b-stable.tar.gz
	cd libevent-1.4.14b-stable
	./configure --prefix=/opt/libevent
	make
	make install
	#You should have libevent installed in /opt/libevent. To make sure:
	ls -la /opt/libevent
```
	
*mysqlcppconn
```
	apt-get install libmysqlcppconn-dev
```

*mysql database


### Installing
```
CREATE DATABASE ftpotd;
CREATE USER 'user'@'localhost' IDENTIFIED BY 'pass';
GRANT ALL PRIVILEGES ON * . * TO 'user'@'localhost';
FLUSH PRIVILEGES;
```
Ensure the prerequisites are installed, and create the following tables:

```

CREATE TABLE `connections` (`ip_string` varchar(15) DEFAULT NULL,`conid_string` varchar(15) DEFAULT NULL,`close` int(11) DEFAULT NULL,`epoch` int(11) unsigned DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `logins` (`user` varchar(255) DEFAULT NULL,`pass` varchar(255) DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `authed` (`conid_string` varchar(15) DEFAULT NULL,`user` varchar(255) DEFAULT NULL,`pass` varchar(255) DEFAULT NULL,`epoch` int(11) unsigned DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `attempts` (`conid_string` varchar(15) DEFAULT NULL,`user` varchar(255) DEFAULT NULL,`pass` varchar(255) DEFAULT NULL,`epoch` int(11) unsigned DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE `activity` (`conid_string` varchar(15) DEFAULT NULL,`data` varchar(4096) DEFAULT NULL,`vfs` varchar(16) DEFAULT NULL,`epoch` int(11) unsigned DEFAULT NULL) ENGINE=InnoDB DEFAULT CHARSET=latin1;
```

Compleate the table of logins:

```
INSERT INTO `logins` VALUES ('root','q1w2e3r4t5y6'),('root','Passw0rd!'),('ftp','itsasecret'),('ftp','12345678'),('letmein','letmein'),('test','developers');
```

Anonymous logins can be enabled by modifying the source code, sockets.cpp:283-340 contains the USER and PASS commands. It should be a trivial implementation.

Set up the credentials for the database in main.cpp:116 -
```
asyncsock *asock = new asyncsock(new cdb("127.0.0.1", "**user**", "**pass**", "**db**"), &virtual_fs, 21);
```

Run it, login and play around! Break it, fix it, set it online in a sandbox... Learn from it. 

## Implemented commands

There is a very basic implementation of the FTP protocol within this code, mostly adhering to the specification, but not totally...

**USER**
**PASS**
**PASV**
**PORT**
**CWD**
**PWD**
**CDUP**
**LIST**
**SYST**

You can find all the networking, and file commands, within sockets(.hpp|.cpp).

## Contributing

Feel free to fork this and work on it! I would be happy to see your changes, no matter how small, or large. :)

## Authors

* **snoom** - *Initial work*

## License

This project is licensed under the GNU PL V3 license - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* Google
* Stackoverflow
* Niels Provos & Nick Matthewson (libevent developers - http://www.wangafu.net/~nickm/)
* MySQL documentation and provided C++ library.
* Coffee. Oh glorious coffee.
* My friends for believing in me to get the project done and the resulting grade I received. They never doubted me. Thanks!!
