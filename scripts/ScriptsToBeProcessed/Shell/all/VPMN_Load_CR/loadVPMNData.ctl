load data
$LOADTYPE
into table OUTROAMER_GPRS_VPMN_RATES
fields terminated by '|'
OPTIONALLY ENCLOSED BY '"'
trailing nullcols
(
 VPMN,
 VPMN_Type "UPPER(:VPMN_TYPE)",
 PULSE,
 VALUE
)
