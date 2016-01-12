#!/bin/bash

git stash push &> /dev/null;
git checkout glstm &> /dev/null;

make clean &> /dev/null;
make &> /dev/null;

mv bank bank_glstm &> /dev/null;
mv ll ll_glstm &> /dev/null;

git checkout master &> /dev/null;
git stash pop &> /dev/null;

echo "!! created bank_glstm and ll_glstm executables."

make clean &> /dev/null;
make > /dev/null;

if [ $? -eq 0 ];
then
    echo "!! created bank and ll executables -- your algorithm"
    exit 0;
else
    echo "!! ERROR creating bank and ll executables -- your algorithm"
    exit 1;
fi
