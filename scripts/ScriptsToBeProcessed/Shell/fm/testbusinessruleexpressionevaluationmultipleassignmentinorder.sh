sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from businessrule where businessrule_name = 'TEST_EXPR_RULES' ;
delete from expression_rule  ;
delete from expression_conditions ;
delete from expression_evaluation_actions ;

insert into businessrule values ('MTX12LNP',1,7,'TEST_EXPR_RULES', 'brexpressionevaluation') ;
insert into expression_rule values ('TEST_EXPR_RULES','TEST_ASSMT_IN_ORDER','1');
insert into expression_conditions values ('TEST_ASSMT_IN_ORDER',1,'','','FIELD','C','.EQ.','123','') ;
insert into expression_evaluation_actions values ('TEST_ASSMT_IN_ORDER','IF','FIELD0','{FIELD2}') ;
insert into expression_evaluation_actions values ('TEST_ASSMT_IN_ORDER','IF','FIELD2','{FIELD1}') ;
insert into expression_evaluation_actions values ('TEST_ASSMT_IN_ORDER','IF','FIELD1','{FIELD0}') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
