SET SERVEROUTPUT ON
SET FEEDBACK ON

create or replace procedure sync_seq (seqName in varchar2,tableName in varchar2)
is
    seq_val     pls_integer;
    id_val      pls_integer;
    diff        pls_integer;
    inc         pls_integer;
begin
    execute immediate 'select '|| seqName ||'.nextval from dual' into seq_val;
    execute immediate 'select max(id) from '||tableName||''  into id_val;
    select increment_by into inc from user_sequences where sequence_name = upper(seqName) ;
    diff := id_val - seq_val;
    if diff > 0
    then
        execute immediate 'alter sequence  '|| seqName ||' increment by ' ||to_char(diff);
        execute immediate 'select '|| seqName ||'.nextval from dual' into seq_val;
        execute immediate 'alter sequence  '|| seqName ||' increment by ' || to_char(inc);
        execute immediate 'select '|| seqName ||'.nextval from dual' into seq_val;
    end if;
end;
/
