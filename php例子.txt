<?
//获取跟男士有共同答案的女士
function manHasAnsweredMatch($manid,$num='8'){
	global $mqdb_read,$cursiteid;
	$newnum=$num+30; //多获取4个，避免女士数量不够
	$manAnswerList=getManAnswer($manid);
	foreach($manAnswerList as $val){
		$manAnswerStr .="'".$val['q_concat']."',";	
	}
	$manAnswerStr = substr($manAnswerStr,0,-1);
	//随机起始值
	$query="select count(DISTINCT(womanid)) as total from  mq_woman_answer  where  q_concat IN (".$manAnswerStr.") AND question_status='1' AND  siteid = '".$cursiteid."'   ";
	$result=mysql_query($query,$mqdb_read);
	$d=mysql_fetch_array($result);
	$total=$d['total'];
	$start=$total<$newnum?0:$total-$newnum+0;
	$rand=mt_rand(0,$start); 
	
	//相同答案数量
	
	$query="select count(id) as  same_answers_num,womanid from  mq_woman_answer  where  q_concat IN(".$manAnswerStr.")  AND question_status='1' AND  siteid = '".$cursiteid."' GROUP BY womanid  limit ".$rand.",".$newnum; 
	$result=mysql_query($query,$mqdb_read);
	$data = array();
	while($info=mysql_fetch_array($result)){ 
		$data[$info['womanid']]['same_answers_num']=$info['same_answers_num'];
		$womanlist[]=$info['womanid'];			
	}
	

	//共同回答问题数量
	/*$woman_str = array2str($womanlist);
	$query="select count(m.id) as  same_question_num,womanid,womanid from mq_man_answer m left join mq_woman_answer w ON m.q_id=w.q_id  where   m.question_status='1' AND m.manid='".$manid."'  AND w.womanid IN (".$woman_str.")  GROUP BY womanid  ";  
	$result=mysql_query($query,$mqdb);
	while($info2=mysql_fetch_array($result)){
		$data[$info2['womanid']]['same_question_num']=$info2['same_question_num'];		
	}*/
	//女士资料
	$data['womaninfo']=array();
	$WomanInfo=array();
	if($total>0){
		$womanlist = sortByCondition($womanlist);	
		$WomanInfo=getWomanInfo($womanlist);
		$data['womaninfo']=array_slice($WomanInfo,0,$num);
	}
	
	return $data;
}
?>