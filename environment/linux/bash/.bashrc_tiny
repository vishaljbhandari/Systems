###################################################################################################
# This is a very simple .bashrc file that I'm using on a daily basis.                             #
# It completely replaced my zsh setup, and should be relatively simple to understand and modify.  #
#                                                                                                 #
# Built by Moritz (mo-mar.de) - just do whatever you want with it, according to the Unlicense:    #
# https://choosealicense.com/licenses/unlicense/                                                  #
#                                                                                                 #
# Simple installation:                                                                            #
# wget https://go.momar.de/bashrc -O ~/.bashrc                                                    #
###################################################################################################

# If not running interactively, don't do anything
case $- in
    *i*) ;;
      *) return;;
esac

###########
## Color ##
###########

# Normal Bash
export PS1='\[\e[1;38;5;244m\]\t \[\e[1;36m\]\u@\H \[\e[1;33m\]\w \[\e[1;36m\]\$ \[\e[0m\]' 

# Alpine Linux / ash
# export PS1='\[\e[1;38;5;244m\]$(date +%H:%M:%S) \[\e[1;36m\]\u@\H \[\e[1;33m\]\w \[\e[1;36m\]\$ \[\e[0m\]'

# Termux (without user@host)
# export PS1='\[\e[1;38;5;244m\]\t \[\e[1;33m\]\w \[\e[1;36m\]\$ \[\e[0m\]'

# Minimal without path to working directory (~ $)
# export PS1='\[\e[1;33m\]\W \[\e[1;36m\]\$ \[\e[0m\]'

##################################
## ls, exa & more colored stuff ##
##################################

if which exa >/dev/null; then
	# exa is a modern ls replacement with Git integration: https://the.exa.website
	alias ls="exa --git-ignore"
	alias ll="exa --git-ignore --git -l --group"
	alias la="exa --git -la"
else
	alias ls="ls --color=always"
	alias ll="ls -l"
	alias la="ls -lA"
fi
for alias in lsl sls lsls sl l s; do alias $alias=ls; done

# colored GCC warnings and errors
export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'

########################################
## Cool bash features nobody knows of ##
########################################

# search through history with up/down arrows
bind '"\e[A": history-search-backward' 2>/dev/null
bind '"\e[B": history-search-forward' 2>/dev/null

# don't put duplicate lines or lines starting with space in the history.
# See bash(1) for more options
HISTCONTROL=ignoreboth

# append to the history file, don't overwrite it
shopt -s histappend

# for setting history length see HISTSIZE and HISTFILESIZE in bash(1)
HISTSIZE=1000
HISTFILESIZE=2000

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# If set, the pattern "**" used in a pathname expansion context will
# match all files and zero or more directories and subdirectories.
shopt -s globstar

####################################################################
## Type :W in vim (or :WQ respectively) to save a file using sudo ##
####################################################################

if which vim >/dev/null && ! grep '^command W ' ~/.vimrc >/dev/null 2>&1 && ! [ `id -u` -eq 0 ]; then
	echo "command W :execute ':silent w !sudo tee % > /dev/null' | :if v:shell_error | :edit! | :endif" >> ~/.vimrc
fi

#############################
## Warn about root shells! ##
#############################

if [ `id -u` -eq 0 ]; then 
    start="\033[1;37;41m"
    end="\033[0m"
    printf "\n"
    printf "  $start                                                                       $end\n"
    printf "  $start  WARNING: You are in a root shell. This is probably a very bad idea.  $end\n"
    printf "  $start                                                                       $end\n"
    printf "\n"
fi

#########################
## Path & Applications ##
#########################

# Setup GOPATH
export GOPATH="$HOME/.local/lib/go"
export PATH="$GOPATH/bin:$PATH"

# Setup npm global installs without sudo
export NPMPATH="$HOME/.local/lib/npm"
export PATH="$NPMPATH/bin:$PATH"
[ -f ~/.npmrc ] || ! which npm >/dev/null || echo "prefix=$NPMPATH" > ~/.npmrc

# Setup git author
[ -f ~/.gitconfig ] || ! which git >/dev/null || git config --global --edit

# Add ~/.local/bin to $PATH
export PATH="$HOME/.local/bin:$PATH"

#############################
## Awesome online services ##
#############################

# Send command output to qbin.io for quick & easy sharing (stored for 14 days)
# Usage: echo "Hello World" | qbin
alias qbin="curl https://qbin.io -s -T -"

# Upload bigger & binary files to transfer.sh (stored for 14 days)
# transfer anything.tar.gz
transfer() { if [ $# -eq 0 ]; then echo -e "No arguments specified. Usage:\necho transfer /tmp/test.md\ncat /tmp/test.md | transfer test.md"; return 1; fi; tmpfile=$( mktemp -t transferXXX ); if tty -s; then basefile=$(basename "$1" | sed -e 's/[^a-zA-Z0-9._-]/-/g'); curl --progress-bar --upload-file "$1" "https://transfer.sh/$basefile" >> $tmpfile; else curl --progress-bar --upload-file "-" "https://transfer.sh/$1" >> $tmpfile ; fi; cat $tmpfile; rm -f $tmpfile; }

###########################
## Other helpful aliases ##
###########################

# If ag is not installed, alias it to "grep -rn" (and generally force color for grep)
alias grep="grep --color=always"
which ag >/dev/null || alias ag="grep -rn"

# Provide a yq command to use jq with YAML files
alias yq="python3 -c 'import sys, yaml, json; json.dump(yaml.load(sys.stdin), sys.stdout, indent=4)' | jq"

# A really simple password generator
alias pw='bash -c '"'"'echo `tr -dc $([ $# -gt 1 ] && echo $2 || echo "A-Za-z0-9") < /dev/urandom | head -c $([ $# -gt 0 ] && echo $1 || echo 30)`'"'"' --'

# View pressure stall information
alias pressure="grep -n '[^ ]*=' /proc/pressure/*"

###########################
## Ubuntu-specific stuff ##
###########################

# make less more friendly for non-text input files, see lesspipe(1)
[ -x /usr/bin/lesspipe ] && eval "$(SHELL=/bin/sh lesspipe)"

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if ! shopt -oq posix; then
  if [ -f /usr/share/bash-completion/bash_completion ]; then
    . /usr/share/bash-completion/bash_completion
  elif [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
  fi
fi

# Ubuntu already had an "fd" package, so the one I'd like to use is called "fdfind".
! which fdfind >/dev/null || alias fd=fdfind

##################
## Custom stuff ##
##################

# I have a few environment variables (paths for Go & node.js) set in environment.d - I'd like to also use them in the terminal.
if [ "$(ls ~/.config/environment.d/ 2>/dev/null)" != "" ]; then
  for f in ~/.config/environment.d/*; do
    source "$f"
  done
fi

# I'm using Tilix, which requires sourcing this script
if [ $TILIX_ID ] || [ $VTE_VERSION ]; then
  if [ -e /etc/profile.d/vte.sh ]; then
    source /etc/profile.d/vte.sh
  fi
fi
