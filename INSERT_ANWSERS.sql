DELIMITER $$

USE `qpidnetwork`$$

DROP PROCEDURE IF EXISTS `INSERT_ANWSERS`$$

CREATE DEFINER=`root`@`127.0.0.1` PROCEDURE `INSERT_ANWSERS`()
BEGIN
	DECLARE id VARCHAR(64);
	DECLARE i INT;
	DECLARE j INT;
	DECLARE r INT;
	DECLARE qid VARCHAR(64);
	DECLARE aid VARCHAR(64);
	
	TRUNCATE TABLE mq_man_answer;
	TRUNCATE TABLE mq_woman_answer;
	SET i = 0;
	WHILE i < 40000 DO
	    SET id = CONCAT('CM', i);
	    SET j = 0;
	    WHILE j < 30 DO
		SET qid = CONCAT('QA', j);
		/* Insert Man */
		SET r = ROUND(RAND() * 100 % 30, 0);
		SET aid = CONCAT(qid, '-', r);
		INSERT INTO mq_man_answer(`q_id`, `question_status`, `content`, `manid`, `manname`, `country`, `answer_id`, `answer`, `q_concat`, `answer_time`, `last_update`, `ip`, `siteid`) VALUES(qid, 1, 'content', id, 'manname', 'country', 1, 'anwser', aid, NOW(), NOW(), '192.168.1.1', 1);
		
		/* Insert Lady */
		SET r = ROUND(RAND() * 100 % 30, 0);
		SET aid = CONCAT(qid, '-', r);
		INSERT INTO mq_woman_answer(`q_id`, `question_status`, `content`, `womanid`, `womanname`, `agent`, `answer_id`, `answer`, `q_concat`, `answer_time`, `last_update`, `siteid`) VALUES(qid, 1, 'content', id, 'womanname', 'agent', 1, 'anwser', aid, NOW(), NOW(), 1);
		
		SET j = j + 1;
	    END WHILE;
	    SET i = i + 1;
	END WHILE;
    END$$

DELIMITER ;