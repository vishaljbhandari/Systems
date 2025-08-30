 sqlldr userid=lalithk/lalithk control=load.ctl \
                log=load.log \
                data=accumulated.log \
                bad=load.bad \
				ERRORS = 999999
