#! /bin/bash
# this script build a file 
# that contains the directories's tree
# for the response of a list command

dir="./root"

touch "tree"
tree $dir > "tree"


