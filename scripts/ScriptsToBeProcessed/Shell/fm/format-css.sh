File=$1

cat $File | 
	paste -s -d" " | 
	sed "s/\*\+/\*/g"  | 
	sed "s/\/\*/\n\/\*/g" | 
	sed "s/\*\//*\/\n/g" |
	sed "s/\/\*\(.*\){\(.*\)}\(.*\)\*\///g" | 
	sed "s/\/\*\(.*\)opyright\(.*\)\*\///g" | 
	sed "s/\/\*\///g" |
	sed "s/\t/\ /g" |
	sed "s/;/\ ;\n/g" |  
	sed "s/[}{]/\n&\n/g" |	
	sed "s/\ *:\ */:/g" | 
	sed "s/^\ \+//g"  | 
	sed "s/\ \+/\ /g" | 
	sed "s/\ \+$//g" |
	sed "s/\(.*\):\(.*\);/\ \ \ \ \1:\2;/g" | 
	sed "s/\ \+;/\ ;/g" | 
	grep -v "^$" |
	sed "s/}/}\n/g"
