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

```
c++11
libevent
mysqlcppconn
mysql database
```

### Installing

Ensure the prerequisites are installed, and create the following tables:

```
-- MySQL dump 10.16  Distrib 10.2.13-MariaDB, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: ftpotd
-- ------------------------------------------------------
-- Server version	10.2.13-MariaDB-10.2.13+maria~stretch-log

DROP TABLE IF EXISTS `activity`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `activity` (
  `conid_string` varchar(15) DEFAULT NULL,   // connection identifier
  `data` varchar(4096) DEFAULT NULL,         // command sent
  `vfs` varchar(16) DEFAULT NULL,            // path from which command occured
  `epoch` int(11) unsigned DEFAULT NULL      // epoch time of command
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `attempts`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `attempts` (
  `conid_string` varchar(15) DEFAULT NULL,   // attempt connection identifier
  `user` varchar(255) DEFAULT NULL,          // attempted user
  `pass` varchar(255) DEFAULT NULL,          // attempted pass
  `epoch` int(11) unsigned DEFAULT NULL      // epoch time of attempt
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `authed`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `authed` (
  `conid_string` varchar(15) DEFAULT NULL,  // authed connection identifier
  `user` varchar(255) DEFAULT NULL,         // authed username
  `pass` varchar(255) DEFAULT NULL,         // authed password
  `epoch` int(11) unsigned DEFAULT NULL     // epoch time of authentication
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `connections`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `connections` (
  `ip_string` varchar(15) DEFAULT NULL,    // IP address of connection
  `conid_string` varchar(15) DEFAULT NULL, // random connection identifier
  `close` int(11) DEFAULT NULL,            // boolean to indicate connection close
  `epoch` int(11) unsigned DEFAULT NULL    // epoch time of interaction
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


DROP TABLE IF EXISTS `logins`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `logins` (
  `user` varchar(255) DEFAULT NULL,  // accepted usernames
  `pass` varchar(255) DEFAULT NULL   // accepted passwords
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

```

Populate the logins table, an example:

```
INSERT INTO `logins` VALUES ('root','q1w2e3r4t5y6'),('root','Passw0rd!'),('ftp','itsasecret'),('ftp','12345678'),('letmein','letmein'),('test','developers');
```

Anonymous logins can be enabled by modifying the source code, sockets.cpp:283-340 contains the USER and PASS commands. It should be a trivial implementation.

Set up the credentials for the database in main.cpp:116 -
```
asyncsock *asock = new asyncsock(new cdb("127.0.0.1", "**user**", "**pass**", "**db**"), &virtual_fs, 21);
```

## Contributing

Feel free to fork this and work on it! I would be happy to see your changes, no matter how small, or large. :)

## Authors

* **snoom** - *Initial work*

## License

This project is licensed under the GNU PL V3 license - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Google
* Stackoverflow
* Niels Provos & Nick Matthewson (libevent developers - http://www.wangafu.net/~nickm/)
* MySQL documentation and provided C++ library.
* Coffee. Oh glorious coffee.
* My friends for believing in me to get the project done and the resulting grade I received. They never doubted me. Thanks!!
