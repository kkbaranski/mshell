#!/bin/bash
########################################################################
#                                                                      #
#                            CREATE ARCHIVE                            #
#                                                                      #
########################################################################
USAGE="
Usage: ./$0
Creates shell.tar.gz archive with mshell
"
if [ $1 = "--help" ]; then
	echo "$USAGE"
	exit 0
fi

archive_name="shell"
files="include/* input_parse/* test/* *.h *.c $(basename $0)"

echo "files='$files'"

if [ -f "$archive_name.tar.gz" ]; then
	rm "$archive_name.tar.gz"
fi

tar -cvzf "$archive_name.tar.gz" $files
