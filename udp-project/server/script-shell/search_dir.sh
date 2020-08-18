#! /bin/bash
#this shell script return 1 if the $1 filename there is in the subdirs
# else return 0

filename=$1


temp=$(find ./root | grep $filename )

#in temp file there's the dir tree
#finded in file content the string filename

temp=${temp:2}
echo $temp
