#/*******************************************************************************
# *  Copyright (c) Subex Limited 2013. All rights reserved.                     *
# *  The copyright to the computer program(s) herein is the property of Subex   *
# *  Limited. The program(s) may be used and/or copied with the written         *
# *  permission from Subex Limited or in accordance with the terms and          *
# *  conditions stipulated in the agreement/contract under which the program(s) *
# *  have been supplied.                                                        *
# *******************************************************************************/
#!/bin/bash

string='ｗｈａｔｅ́ｖｅｒ'
# aka string=$'\uFF57\uFF48\uFF41\uFF54\uFF45\u0301\uFF56\uFF45\uFF52'
stty size | {
  read y x
  tput sc # save cursor position
  tput cup "$((y - 1))" 0 # position cursor
  printf "%${x}Ls" "$string"
  tput rc # restore cursor.
}

