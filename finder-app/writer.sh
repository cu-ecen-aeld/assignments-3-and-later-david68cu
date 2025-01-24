#!/bin/bash

# Check if two arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments required. Usage: $0 <writefile> <writestr>"
    exit 1
fi

# Assign arguments to variables
writefile=$1
writestr=$2

# Create the directory path if it doesn't exist
dir_path=$(dirname "$writefile")
if [ ! -d "$dir_path" ]; then
    mkdir -p "$dir_path" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create directory path $dir_path"
        exit 1
    fi
fi

# Write the string to the file (overwrite if it exists)
echo "$writestr" > "$writefile" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Error: Failed to write to file $writefile"
    exit 1
fi

echo "Successfully wrote to $writefile"
exit 0

