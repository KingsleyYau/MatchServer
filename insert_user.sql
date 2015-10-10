DELIMITER $$

CREATE
    /*[DEFINER = { user | CURRENT_USER }]*/
    PROCEDURE `qpidnetwork`.`insert_user`()
    /*LANGUAGE SQL
    | [NOT] DETERMINISTIC
    | { CONTAINS SQL | NO SQL | READS SQL DATA | MODIFIES SQL DATA }
    | SQL SECURITY { DEFINER | INVOKER }
    | COMMENT 'string'*/
    BEGIN
	DECLARE id CHAR(10);
	DECLARE aid INT;
	DECLARE qid INT;

	SET id = 0;
	WHILE  id < 40000 DO
		SET qid = 0;
		WHILE  qid < 30 DO
			SET aid = ROUND(RAND()*30);
			INSERT INTO `mq_man_answer`(`q_id`,`question_status`,`content`,`manid`,`manname`,`country`,`answer_id`,`answer`,`q_concat`,`answer_time`,`last_update`,`ip`,`siteid`) VALUES(qid,1,'content',id,'name','country',1,'answer',aid,CURDATE(),CURDATE(),'ip',1);
			INSERT INTO `mq_woman_answer`(`q_id`,`question_status`,`content`,`womanid`,`womanname`,`agent`,`answer_id`,`answer`,`q_concat`,`answer_time`,`last_update`,`siteid`) VALUES(qid,1,'content',id,'name','agent',1,'answer',aid,CURDATE(),CURDATE(),1);
			SET qid = qid + 1;
		END WHILE;
		SET id = id + 1;
	END WHILE;
    END$$

DELIMITER ;