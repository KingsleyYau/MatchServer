<?
//1.
//获取跟男士有共同答案的女士
function manHasAnsweredMatch2($manid,$num='8'){
	return array();
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
	$result=mysql_query($query,$mqdb_read);
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




//2.
//获取回答过该题的女士
//$num :女士個數
function  getWomanByQustion($q_id,$num='6'){
	global $mqdb,$dbh,$cursiteid;
	
	$newnum=$num+30; //多获取30个，避免女士数量不够
	//随机起始值
	$query="select count(id) as total from mq_woman_answer   WHERE siteid = '".$cursiteid."' AND q_id ='".$q_id."' ";
	$result=mysql_query($query,$mqdb);
	$d=mysql_fetch_array($result);
	$total=$d['total'];
	$start=$total<$newnum?0:$total-$newnum+0;
	$rand=mt_rand(0,$start); 
	
	$query="select womanid from mq_woman_answer   WHERE siteid = '".$cursiteid."' AND q_id ='".$q_id."' limit ".$rand.",".$newnum;
	$result=mysql_query($query,$mqdb);
	
	$womanlist=array();
	if($total>0){
		while($qinfo=mysql_fetch_array($result)){
			$womanlist[]=$qinfo['womanid'];		
		}
		$womanlist = sortByCondition($womanlist);
		$WomanInfo = getWomanInfo($womanlist);
		$winfoArr=array_slice($WomanInfo,0,$num);
		
		return $winfoArr;		
		
	}else{
	  return false;
	}
	
}



//3.
//注册时候获取推荐女士
function getPromotionWoman ($qidArr){
	global $dbh,$cursiteid,$mqdb;
		
	$q_num= count($qidArr);	
	//在线女士
	$query="select womanid  from  woman  where   deleted='0' and status1 ='0' and  online_status='1'";
	$result=mysql_query($query,$dbh);
	$onlinelist=array();	
	while($winfo=mysql_fetch_array($result)){
		$onlinelist[]=$winfo['womanid'];		
	}
	
	//根据在线排序
	$online_str = array2str($onlinelist);
	$qid_str = array2str($qidArr);
	$womaninfo = array();
	$womanlist=array();
	if(strlen($qid_str)>0 && strlen($online_str)>0){
		//随机起始值
		 $num = 20;
		 $query="select count(DISTINCT(womanid)) as total from mq_woman_answer  WHERE siteid = '".$cursiteid."' AND q_id IN (".$qid_str.") AND womanid IN (".$online_str.")";
		$result=mysql_query($query,$mqdb);
		$d=mysql_fetch_array($result);
		$total=$d['total'];
		$start=$total<$num?0:$total-$num+0;
		$rand=mt_rand(0,$start); 
	
		
		//回答过注册问题的在线女士
		 $query="select  DISTINCT(womanid) from mq_woman_answer  WHERE siteid = '".$cursiteid."' AND q_id IN (".$qid_str.") AND womanid IN (".$online_str.") limit ".$rand.",".$num;;
		$result=mysql_query($query,$mqdb);
		$qwoman_num=mysql_num_rows($result);
		if($qwoman_num>0){
			while($qinfo=mysql_fetch_array($result)){
				$womanlist[]=$qinfo['womanid'];		
			}
		}
		$need_num =  $num-$qwoman_num;
		if($need_num>0){
			shuffle($onlinelist);
			for($i=0;$i<$need_num;$i++){
				if(strlen($onlinelist[$i])>0 && !in_array($onlinelist[$i],$womanlist)){
					$womanlist[]=$onlinelist[$i];
				}	
			}	
		}			
	}

	
	$womanlist=sortByCondition($womanlist);
	
	$womanlist=array_slice($womanlist,0,4);
	
	$need_num = 4-count($womanlist);
	$not_in_str = array2str($womanlist);
	$sql='';
	if(strlen($not_in_str)>0){
		$sql.="  and womanid NOT IN (".$not_in_str.") ";
	}
	if($need_num >0){
		//有相同答案的女士
		$query="select w.womanid from mq_man_answer m left join mq_woman_answer w ON m.q_id=w.q_id  where  m.answer_id=w.answer_id  AND m.question_status='1' AND  w.siteid = '".$cursiteid."' AND m.manid='".$_SESSION['_sessUser']['reg_userid']."' ".$sql." GROUP BY womanid  limit ".$need_num;
		$result=mysql_query($query,$mqdb);
		$qwoman_num=mysql_num_rows($result);
		while($qinfo=mysql_fetch_array($result)){
				$womanlist[]=$qinfo['womanid'];		
			}	
	}
	
	$need_num = 4-count($womanlist);
	$not_in_str = array2str($womanlist);
	$sql='';
	if(strlen($not_in_str)>0){
		$sql.="  and womanid NOT IN (".$not_in_str.") ";
	}
	if($need_num >0){
		//回答过相同问题女士
		$query="select w.womanid from mq_man_answer m left join mq_woman_answer w ON m.q_id=w.q_id  where  m.question_status='1' AND  w.siteid = '".$cursiteid."'  AND m.manid='".$_SESSION['_sessUser']['reg_userid']."'  ".$sql." GROUP BY womanid  limit ".$need_num;
		$result=mysql_query($query,$mqdb);
		$qwoman_num=mysql_num_rows($result);
		while($qinfo=mysql_fetch_array($result)){
				$womanlist[]=$qinfo['womanid'];		
			}
	}
	$womaninfo=getWomanInfo($womanlist);
	return $womaninfo;
}
?>