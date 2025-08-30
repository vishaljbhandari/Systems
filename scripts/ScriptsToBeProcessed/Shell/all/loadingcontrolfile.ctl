load data
$LOADTYPE
into table tablename
fields terminated by '|'
OPTIONALLY ENCLOSED BY '"'
trailing nullcols
(
 field1 "UPPER(:field1)",
 field2,
 field3,
 field4
)
