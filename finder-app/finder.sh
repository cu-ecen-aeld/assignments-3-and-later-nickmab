#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "Usage: finder.sh <filesdr> <searchstr>"
    exit 1
fi

FILESDR=$1
SEARCHSTR=$2

if [ ! -d "${FILESDR}" ]; then
    echo "Directory '${FILESDR}' was not found."
    exit 1
fi

FILECOUNT=$(find "${FILESDR}" -type f | wc -l)
MATCHCOUNT=$(grep -r "${SEARCHSTR}" "${FILESDR}" | wc -l)

echo "The number of files are ${FILECOUNT} and the number of matching lines are ${MATCHCOUNT}"