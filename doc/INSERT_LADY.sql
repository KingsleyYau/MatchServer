DELIMITER $$

USE `qpidnetwork_online`$$

DROP PROCEDURE IF EXISTS `INSERT_LADY`$$

CREATE DEFINER=`root`@`127.0.0.1` PROCEDURE `INSERT_LADY`()
BEGIN
	DECLARE womanid VARCHAR(32);
	DECLARE i INT;
	TRUNCATE TABLE online_woman;
	SET i = 0;
	WHILE i < 5000 DO
	    SET womanid = CONCAT('CM', i);
	    INSERT INTO online_woman(`womanid`, `lastlogin`) VALUES(womanid, NOW());
	    SET i = i + 1;
	END WHILE;
    END$$

DELIMITER ;