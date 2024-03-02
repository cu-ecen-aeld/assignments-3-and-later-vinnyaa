#!/bin/bash


# verify the number of arguments that have been passed in
if [ $# -ne 2 ]
then
	echo "Incorrect number of arguments entered"
	exit 1
fi

# saving the arguments to readable variables
filesdir=$1
searchstr=$2

# verify the given path is actually a directory
if ! [ -d $filesdir ]
then
	echo "Provided path is not a directory"
	exit 1
fi

# create variables to track number of files, 
# number of lines, and string matches
numfiles=$(ls $filesdir -1 | wc -l)

numhits=$(grep -R "$searchstr" "$filesdir" | wc -l)

echo "The number of files are "$numfiles" and the number of matching lines are "$numhits""
