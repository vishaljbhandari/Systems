load data
$LOADTYPE
into table OUTROAMER_GPRS_VPMN_RATES
fields terminated by '|'
OPTIONALLY ENCLOSED BY '"'
trailing nullcols
(
 VPMN "UPPER(:VPMN)",
 VPMN_Type,
 PULSE,
 VALUE
)
