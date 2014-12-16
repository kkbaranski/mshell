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

if [[ $1 = "--help" ]]; then
	echo "$USAGE"
	exit 0
fi

archive_name="shell"
files="include/* input_parse/* test/* *.h *.c Makefile $(basename $0)"

if [ -f "$archive_name.tar.gz" ]; then
	rm "$archive_name.tar.gz"
fi

dirnam="${PWD##*/}"
paths=""

for name in $files; do
	paths="$paths $dirnam/$name"
done

parent="$( readlink -f ".." )"

tar -C "$parent" -cvzf "$archive_name.tar.gz" $paths --transform s/$dirnam/shell/
