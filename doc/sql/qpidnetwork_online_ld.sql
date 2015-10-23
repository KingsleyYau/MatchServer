/*
SQLyog Ultimate v11.27 (32 bit)
MySQL - 5.6.27 : Database - qpidnetwork_online_ld
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE DATABASE /*!32312 IF NOT EXISTS*/`qpidnetwork_online_ld` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `qpidnetwork_online_ld`;

/*Table structure for table `online_woman` */

DROP TABLE IF EXISTS `online_woman`;

CREATE TABLE `online_woman` (
  `id` int(8) NOT NULL AUTO_INCREMENT,
  `womanid` varchar(12) NOT NULL,
  `firstname` varchar(20) DEFAULT NULL,
  `lastname` varchar(20) DEFAULT NULL,
  `cnname` varchar(60) DEFAULT NULL,
  `agentid` varchar(10) DEFAULT NULL,
  `siteid` tinyint(1) DEFAULT NULL,
  `photourl` varchar(100) DEFAULT NULL,
  `birthday` date DEFAULT NULL,
  `description` varchar(150) DEFAULT NULL,
  `livein` varchar(50) DEFAULT NULL,
  `country` varchar(20) DEFAULT NULL,
  `status` tinyint(1) DEFAULT NULL,
  `chatstatus` tinyint(1) DEFAULT NULL,
  `lastlogin` datetime DEFAULT NULL,
  `adddate` datetime DEFAULT NULL,
  `height` int(3) DEFAULT NULL,
  `weight` int(3) DEFAULT NULL,
  `mic` tinyint(1) DEFAULT NULL,
  `cam` tinyint(1) DEFAULT NULL,
  `sid` varchar(50) DEFAULT NULL,
  `phpsession` varchar(50) DEFAULT NULL,
  `chatwin` tinyint(1) DEFAULT '0',
  `videofile` varchar(60) DEFAULT NULL,
  `keyword` varchar(30) DEFAULT NULL,
  `keyword_d` varchar(30) DEFAULT NULL,
  `hide` tinyint(1) DEFAULT '0',
  `woman_auto_id` int(8) DEFAULT NULL,
  `hascam` tinyint(1) DEFAULT NULL,
  `camisopen` tinyint(1) DEFAULT NULL,
  `binding` tinyint(4) DEFAULT NULL,
  `translator_id` varchar(12) DEFAULT NULL,
  `translator_name` varchar(30) DEFAULT NULL,
  `translator_cnname` varchar(60) DEFAULT NULL,
  `need_translate` tinyint(4) DEFAULT NULL,
  `fromid` varchar(10) DEFAULT NULL,
  `device_type` tinyint(2) DEFAULT '10',
  `ip` varchar(20) DEFAULT NULL,
  `proxyip` varchar(20) DEFAULT NULL,
  `iplocation` varchar(50) DEFAULT NULL,
  `proxynum` varchar(20) DEFAULT NULL,
  `ordervalue` int(4) DEFAULT '0',
  `zodiac` tinyint(1) unsigned DEFAULT '1',
  PRIMARY KEY (`id`),
  UNIQUE KEY `womanid` (`womanid`),
  KEY `status` (`status`,`hide`,`binding`)
) ENGINE=MyISAM AUTO_INCREMENT=5001 DEFAULT CHARSET=utf8;

/* Procedure structure for procedure `INSERT_LADY` */

/*!50003 DROP PROCEDURE IF EXISTS  `INSERT_LADY` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`127.0.0.1` PROCEDURE `INSERT_LADY`()
BEGIN
	DECLARE womanid VARCHAR(32);
	DECLARE i INT;
	TRUNCATE TABLE online_woman;
	SET i = 0;
	WHILE i < 5000 DO
	    SET womanid = CONCAT('LD', i);
	    INSERT INTO online_woman(`womanid`, `lastlogin`, `status`, `hide`, `binding`, `siteid`) VALUES(womanid, NOW(), 1, 0, 2, 5);
	    SET i = i + 1;
	END WHILE;
    END */$$
DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
