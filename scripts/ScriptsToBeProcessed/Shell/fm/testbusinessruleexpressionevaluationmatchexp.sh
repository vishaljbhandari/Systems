sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from businessrule where businessrule_name = 'TEST_EXPR_RULES' ;
delete from expression_rule  ;
delete from expression_conditions ;
delete from expression_evaluation_actions ;

insert into businessrule values ('MTX12LNP',1,7,'TEST_EXPR_RULES', 'brexpressionevaluation') ;
insert into expression_rule values ('TEST_EXPR_RULES','TEST_MATCH_EXP_COND','1');
insert into expression_conditions values ('TEST_MATCH_EXP_COND',1,'','','FIELD','C','.MX.','^..1.$','') ;
insert into expression_evaluation_actions values ('TEST_MATCH_EXP_COND','IF','NEW_FIELD','FOUND') ;
insert into expression_evaluation_actions values ('TEST_MATCH_EXP_COND','ELSE','NEW_FIELD','NOT_FOUND') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
