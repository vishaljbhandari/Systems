load data
$LOADTYPE
into table TABLE_NAME
fields terminated by '|'
OPTIONALLY ENCLOSED BY '"'
trailing nullcols
(
 F1	"UPPER(:F1)",
 F2,
 F3,
 F4
)
