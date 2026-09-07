#!/bin/sh

# source the file to setup enviroment

# ALIASES
alias run_build_db='cmake -B build -G Ninja -DDEBUG=True'
alias run_build='cmake -B build -G Ninja -DDEBUG=False'

# generates tags for vim, 
alias getTags='ctags -R .'

# FUNCTIONS

## it formats all code in the project
## -o means or, \( \) this means :  ( ), exclude build
## xargs change output of find to args                                                             

function format_code() {

    find . \
        \( -name "*.h" -o -name "*.cc" \) \
        -not -path "./build/*" \
         | xargs clang-format -style=Google -i
    
    echo "Code formatting completed"
}


## VARIABLES
export CMAKE_POLICY_VERSION_MINIMUM=3.11
export GIT_PAGER=cat
