#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: writer.sh <writefile> <writestr>"
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

WRITEDIR=$(dirname "${WRITEFILE}")
mkdir -p "${WRITEDIR}"

if [ "$?" -ne 0 ]; then
    echo "Unable to find or create dir '${WRITEDIR}'"
    exit 1
fi

echo "${WRITESTR}" > "${WRITEFILE}"

if [ "$?" -ne 0 ]; then
    echo "Unable to create or write to file '${WRITEFILE}'"
    exit 1
fi