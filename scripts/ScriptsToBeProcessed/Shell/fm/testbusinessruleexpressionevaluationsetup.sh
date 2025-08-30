sqlplus $DB_USER/$DB_PASSWORD << EOF
whenever sqlerror exit 5;
whenever oserror exit 5;

delete from datasource_info where datasource_name like  '%MTX12LNP%'  ;
delete from businessrule where datasource_name = 'MTX12LNP' ;
delete from expression_rule  ;
delete from expression_conditions ;
delete from expression_evaluation_actions ;

insert into datasource_info values('MTX12LNP',1) ;
insert into businessrule values ('MTX12LNP',1,7,'TEST_EXPR_RULES', 'brexpressionevaluation') ;
insert into expression_rule values ('TEST_EXPR_RULES','TEST_SINGLE_COND','1');
insert into expression_conditions values ('TEST_SINGLE_COND',1,'','','FIELD1','N','.GT.','30','') ;
insert into expression_evaluation_actions values ('TEST_SINGLE_COND','IF','NEW_FIELD','111') ;
insert into expression_evaluation_actions values ('TEST_SINGLE_COND','ELSE','NEW_FIELD','999') ;

commit;
EOF

if test $? -eq 5
then
	exit 1
fi

exit 0
