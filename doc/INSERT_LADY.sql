DELIMITER $$

USE `qpidnetwork_online_1`$$

DROP PROCEDURE IF EXISTS `INSERT_LADY`$$

CREATE DEFINER=`root`@`127.0.0.1` PROCEDURE `INSERT_LADY`()
BEGIN
	DECLARE womanid VARCHAR(32);
	DECLARE i INT;
	TRUNCATE TABLE online_woman;
	SET i = 0;
	WHILE i < 5000 DO
	    SET womanid = CONCAT('CM', i);
	    INSERT INTO online_woman(`womanid`, `lastlogin`, `status`, `hide`, `binding`, `siteid`) VALUES(womanid, NOW(), 1, 0, 2, 1);
	    SET i = i + 1;
	END WHILE;
    END$$

DELIMITER ;