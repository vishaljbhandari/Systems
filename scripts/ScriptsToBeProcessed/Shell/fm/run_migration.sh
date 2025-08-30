#! /bin/bash

sh initialize_migrations.sh
sh migrate_hotlist_groups.sh
sh migrate_nicknames.sh
sh migrate_rater.sh
sh migrate_server_config.sh
sh migrate_subscriber_groups.sh
sh migrate_subscriber.sh
