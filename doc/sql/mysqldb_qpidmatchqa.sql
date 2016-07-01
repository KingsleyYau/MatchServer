/*
SQLyog Ultimate v11.27 (32 bit)
MySQL - 5.6.27 : Database - mysqldb_qpidmatchqa
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
CREATE DATABASE /*!32312 IF NOT EXISTS*/`mysqldb_qpidmatchqa` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `mysqldb_qpidmatchqa`;

/*Table structure for table `mq_man_answer` */

DROP TABLE IF EXISTS `mq_man_answer`;

CREATE TABLE `mq_man_answer` (
  `id` int(8) unsigned NOT NULL AUTO_INCREMENT,
  `q_id` varchar(20) NOT NULL,
  `question_status` tinyint(1) NOT NULL,
  `content` varchar(400) NOT NULL,
  `manid` char(12) NOT NULL,
  `manname` varchar(50) NOT NULL,
  `country` varchar(50) NOT NULL,
  `answer_id` tinyint(1) NOT NULL,
  `answer` varchar(50) NOT NULL,
  `q_concat` varchar(30) NOT NULL,
  `answer_time` datetime NOT NULL,
  `last_update` datetime NOT NULL,
  `ip` varchar(20) NOT NULL,
  `siteid` tinyint(2) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `manid` (`manid`),
  KEY `q_id` (`q_id`)
) ENGINE=MyISAM AUTO_INCREMENT=1200001 DEFAULT CHARSET=utf8;

/*Table structure for table `mq_woman_answer` */

DROP TABLE IF EXISTS `mq_woman_answer`;

CREATE TABLE `mq_woman_answer` (
  `id` int(8) unsigned NOT NULL AUTO_INCREMENT,
  `q_id` varchar(20) NOT NULL,
  `question_status` tinyint(1) NOT NULL,
  `content` varchar(400) NOT NULL,
  `womanid` varchar(50) NOT NULL,
  `womanname` varchar(50) NOT NULL,
  `agent` varchar(20) NOT NULL,
  `answer_id` tinyint(1) NOT NULL,
  `answer` varchar(50) NOT NULL,
  `q_concat` varchar(30) NOT NULL,
  `answer_time` datetime NOT NULL,
  `last_update` datetime NOT NULL,
  `siteid` tinyint(1) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `womanid` (`womanid`),
  KEY `q_id` (`q_id`),
  KEY `q_concat` (`q_concat`)
) ENGINE=MyISAM AUTO_INCREMENT=1200001 DEFAULT CHARSET=utf8;

/* Procedure structure for procedure `INSERT_ANWSERS` */

/*!50003 DROP PROCEDURE IF EXISTS  `INSERT_ANWSERS` */;

DELIMITER $$

/*!50003 CREATE DEFINER=`root`@`127.0.0.1` PROCEDURE `INSERT_ANWSERS`()
BEGIN
	DECLARE manid VARCHAR(64);
	DECLARE womanid VARCHAR(64);
	DECLARE i INT;
	DECLARE j INT;
	DECLARE r INT;
	DECLARE siteid INT;
	DECLARE siteid2 INT;
	DECLARE qid VARCHAR(64);
	DECLARE aid VARCHAR(64);
	
	TRUNCATE TABLE mq_man_answer;
	TRUNCATE TABLE mq_woman_answer;
	SET i = 0;
	WHILE i < 40000 DO
		SET siteid = ROUND(RAND() * 100 % 5, 0);
		SET manid = CONCAT('CM', i);
		SET siteid2 = ROUND(RAND() * 100 % 5, 0);
		IF siteid2 = 0
		THEN
		SET womanid = CONCAT('CL', i);
		ELSEIF siteid2 = 1
		THEN
		SET womanid = CONCAT('IDA', i);
		ELSEIF siteid2 = 2
		THEN  
		SET siteid2 = 0;
		SET womanid = CONCAT('CL', i);
		ELSEIF siteid2 = 3
		THEN
		SET siteid2 = 1;
		SET womanid = CONCAT('IDA', i);
		ELSEIF siteid2 = 4
		THEN 
		SET womanid = CONCAT('CD', i);
		ELSEIF siteid2 = 5
		THEN 
		SET womanid = CONCAT('LD', i);
		END IF;
		
	    SET j = 0;
	    WHILE j < 30 DO
		SET qid = CONCAT('QA', j);
		/* Insert Man */
		SET r = ROUND(RAND() * 100 % 4, 0);
		SET aid = CONCAT(qid, '-', r);
		INSERT INTO mq_man_answer(`q_id`, `question_status`, `content`, `manid`, `manname`, `country`, `answer_id`, `answer`, `q_concat`, `answer_time`, `last_update`, `ip`, `siteid`) VALUES(qid, 1, 'content', manid, 'manname', 'country', r, 'anwser', aid, NOW(), NOW(), '192.168.1.1', siteid);
		
		/* Insert Lady */
		SET r = ROUND(RAND() * 100 % 4, 0);
		SET aid = CONCAT(qid, '-', r);
		INSERT INTO mq_woman_answer(`q_id`, `question_status`, `content`, `womanid`, `womanname`, `agent`, `answer_id`, `answer`, `q_concat`, `answer_time`, `last_update`, `siteid`) VALUES(qid, 1, 'content', womanid, 'womanname', 'agent', r, 'anwser', aid, NOW(), NOW(), siteid2);
		
		SET j = j + 1;
	    END WHILE;
	    SET i = i + 1;
	END WHILE;
    END */$$
DELIMITER ;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
