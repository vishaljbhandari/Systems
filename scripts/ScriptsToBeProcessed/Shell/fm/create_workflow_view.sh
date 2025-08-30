 $1 << EOF

create or replace view alarm_workflow_v as (
    select  wf_t.id, a.name, wf_t.alarm_id, wf_t.activity_id, wf_t.user_id,
            wf_t.expected_completion_date, wf_t.actual_completion_date, wf_t.completed_by, wf_t.status, wf_t.position,
            wf_t.is_mandatory, a.is_automatic,wf_t.workflow_configuration_id
    from activities a, workflow_transactions wf_t
    where a.id = wf_t.activity_id
) ;

create or replace view archived_alarm_workflow_v as (
    select  wf_t.id, a.name, wf_t.alarm_id, wf_t.activity_id, wf_t.user_id,
            wf_t.expected_completion_date, wf_t.actual_completion_date, wf_t.completed_by, wf_t.status, wf_t.position,
            wf_t.is_mandatory, a.is_automatic, wf_t.workflow_configuration_id
    from archived_activities a, archived_workflow_transactions wf_t
    where a.id = wf_t.activity_id
) ;

create or replace view activities_v as (
    select a.id, a.name, a.description, a.jeopardy_period, a.is_automatic, a.activity_template_id, at.name as template_name
    from activities a, activity_templates at
    where a.activity_template_id = at.id
) ;

create or replace view workflow_configuration_v as (
    select p.id, p.workflow_id, p.current_step_id, p.activities_output_map_id,
        p.next_step_id, s.activity_id as next_activity_id, s.position
    from workflow_paths p, workflow_activity_steps s
    where p.workflow_id = s.workflow_id
        and p.next_step_id = s.step_id
) ;

drop sequence workflow_transactions_seq ;
create sequence workflow_transactions_seq increment by 1 start with 1024 nomaxvalue nocycle cache 20 ;

EOF
