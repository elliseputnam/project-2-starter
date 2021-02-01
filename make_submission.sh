#!/bin/bash


RED='\033[0;31m'
NC='\033[0m'
GREEN='\033[0;32m'

make clean && make CC=clang >> compileErrors.txt

if [[ $? -ne 0 ]]; then
    echo -e "${RED}Code did not compile"
    echo -e "${RED}Errors are located in compileErrors.txt"
    exit 1
fi

rm ./compileErrors.txt

test -f "./Makefile"
if [[ $? -ne 0 ]]; then
    echo -e "${RED}No Makefile please create a file called Makefile that compiles your code"
    exit 1
fi

if [ $(hostname) = 'isengard.mines.edu' ]
then
    zip -r $User-submission ./
    echo -e "${GREEN}Zip file successfully created"
    echo -e "${GREEN}Submit $USER-submission to gradescope for the coresponding deliverable"
    exit 0
else
    echo -e "${RED}Please run this command on isengard to create Zip file"
    exit 1
fi
