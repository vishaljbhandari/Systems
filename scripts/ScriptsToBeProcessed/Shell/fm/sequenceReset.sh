sed -i "s#<Sequence SequenceName=\"THRESHOLDS_SEQ\" IncrementValue=\"1\" StartValue=\"3540\"#<Sequence SequenceName=\"THRESHOLDS_SEQ\" IncrementValue=\"1\" StartValue=\"3500\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"AGG_RECORD_CONFIG_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"AGG_RECORD_CONFIG_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"1\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"REGULAR_EXPRESSIONS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"REGULAR_EXPRESSIONS_SEQ\" IncrementValue=\"1\" StartValue=\"1\"#g" schema.xml

sed -i "s#<Sequence SequenceName=\"DBWRITER_CONFIGURATIONS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"DBWRITER_CONFIGURATIONS_SEQ\" IncrementValue=\"1\" StartValue=\"506\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"RANGER_GROUPS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"RANGER_GROUPS_SEQ\" IncrementValue=\"1\" StartValue=\"1024\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"FUNCTIONS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"FUNCTIONS_SEQ\" IncrementValue=\"1\" StartValue=\"58\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"REFERENCE_TYPES_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"REFERENCE_TYPES_SEQ\" IncrementValue=\"1\" StartValue=\"227\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"EXPANDABLE_FIELD_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"EXPANDABLE_FIELD_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"4534\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"TRANSLATION_INDICES_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"TRANSLATION_INDICES_SEQ\" IncrementValue=\"1\" StartValue=\"1560\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"FIELD_CATEGORIES_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"FIELD_CATEGORIES_SEQ\" IncrementValue=\"1\" StartValue=\"6023\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"FIELD_CONFIG_LINKS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"FIELD_CONFIG_LINKS_SEQ\" IncrementValue=\"1\" StartValue=\"103\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"FILE_UPLOAD_CONFIGURATIONS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"FILE_UPLOAD_CONFIGURATIONS_SEQ\" IncrementValue=\"1\" StartValue=\"15\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"PSEUDO_FUNCTIONS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"PSEUDO_FUNCTIONS_SEQ\" IncrementValue=\"1\" StartValue=\"1027\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"ALARM_STATUS_ACTION_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"ALARM_STATUS_ACTION_MAPS_SEQ\" IncrementValue=\"1\" StartValue=\"45\"#g" schema.xml
sed -i "s#<Sequence SequenceName=\"COMPONENTS_SEQ\" IncrementValue=\"1\" StartValue=\"[0-9]*\"#<Sequence SequenceName=\"COMPONENTS_SEQ\" IncrementValue=\"1\" StartValue=\"4\"#g" schema.xml

sed -i "s#Sp0211Paramater#Sp0211Parameter#g" schema.xml
