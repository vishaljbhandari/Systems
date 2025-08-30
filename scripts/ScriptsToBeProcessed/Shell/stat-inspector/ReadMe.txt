Step 1 : Getting git logs
-------------------------
1) Sort file in timestamp and merge into one file
2) read merge file and create master record
3) store master file as ".git_master.txt"

Report 1
git log --pretty=format:"%h#%an#%ae#%cd#%s"

Report 2


Create master record

Date
UserName
UserMailId
Version
CommitTag
FileName
lineStart
LineEnd

Date#UserName#UserMailId#Version#CommitTag#FileName#lineStart#LineEnd




Step 2 : Getting Stat Logs
1) Sort file in timestamp and merge into one file
2) read merge file and create master record
3) store master file as ".stat_master.txt"

Create master record

Project
Module
Priority
Category
FileName
LineNumber
FilePath
Link


Date#UserName#UserMailId#Version#CommitTag#FileName#lineStart#LineEnd






--- Storage Array---
GITCOMMIT - USER - DATE - TAG
GITCOMMIT - FILE NAME - LINES

FILE - ERROR - LINES

ERROR - USER[s]


Step 3 : Creating Reports
R1 : Summary Stats (Later)


R3 : 


Send Mail Reports






Schedule these reports in cron tab after all stat run



















Get All Errors from stat reports
for each error
	o/p : critical ness, category, error name etc
	o/p files involved
		for each file
			o/p : In which all revisions it was edited
				for each rivision
					o/p : date of revision, revision name, modifer name, comment
