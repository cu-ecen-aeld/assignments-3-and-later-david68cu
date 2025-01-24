#!/bin/bash

# Check if two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments required. Usage: $0 <filesdir> <searchstr>"
    exit 1
fi

# Assign arguments to variables
filesdir=$1
searchstr=$2

# Check if filesdir is a directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a valid directory"
    exit 1
fi

# Count the number of files in the directory and its subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of lines containing the search string
match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Output the results
echo "The number of files are $file_count and the number of matching lines are $match_count"
exit 0

