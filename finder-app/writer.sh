# .sh typical opening line
#!/bin/bash



# test the number of arguments
if [ $# -ne 2 ]
then
	echo "Incorrect number of arguments entered"
	exit 1
fi

# purpose driven arg->var capture line
writefile=$1

# purpose driven argument capture of text to be written to file
writestr=$2

# separating the path from the file name to check for directory
writedir="$(dirname "$writefile")"

# test if file/directory does exist, and then will overwrite it
if [ -d "$writedir" ]
then
	# directory exists, just write to / overwrite file
	echo "$writestr" > "$writefile"
	# exit successfully 
	exit 0	
else
	# directory does not exist, make the directory exist
	mkdir -p "$writedir"
	echo "$writestr" > "$writefile"
	exit 0
fi
