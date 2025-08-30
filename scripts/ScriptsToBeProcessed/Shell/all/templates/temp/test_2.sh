    #!/bin/bash

SC_COLUMNS=$(tput cols)
SC_ROWS=$(tput lines)

function ProgressBar {
    let _progress=(${1}*100/${2}*100)/100
    let _done=(${_progress}*4)/10
    let _left=40-$_done
    _fill=$(printf "%${_done}s")
    _empty=$(printf "%${_left}s")

	printf "\rProgress : [${_fill// /#}${_empty// /-}] ${_progress}%%"

}

WELCOME_STRING="Welcome to Subex Release"
while :
do
clear
	tput sgr0
	set_foreground=$(tput setaf 4)
	tput bold
	echo -n $set_background$set_foreground

	printf '%*s\n' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	printf "%*s\n" $(((${#WELCOME_STRING}+$SC_COLUMNS)/2)) "$WELCOME_STRING"
	#printf '#RELESE:4521 #PRODUCT:2415\n'
	printf '%*s\n' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput cup $((SC_ROWS-3)) 0
	printf '%*s' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput cup $((SC_ROWS-2)) 0
	ProgressBar 85 100
	tput cup $((SC_ROWS-1)) 0
	printf '%*s' "${COLUMNS:-$SC_COLUMNS}" '' | tr ' ' '~'
	tput cup 15 15
	read -p "Press [Enter] key to continue..." readEnterKey
	tput sgr0
done
