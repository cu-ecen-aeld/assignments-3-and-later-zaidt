#!/bin/sh

if [ $# -ne 2 ]; then 
    #echo "Usage: $0 FilesDir searchstr"
    echo "invalid arguemnts number, $#"
    exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]; then
    echo "Directory $filesdir does not exist!"
    exit 1
fi

files_num=0
matched_lines_num=0

printdir() {
    for f in $1/* 
    do
        #echo $f
        # exist ?
        if [ -e $f ]; then 
            # is dir?
            if [ -d $f ]; then
                printdir $f
            else
                files_num=$(($files_num + 1))
                file_matched_lines_num=$(grep -c $searchstr $f)
                matched_lines_num=$(($matched_lines_num + $file_matched_lines_num))
            fi
        fi
    done    
}

printdir $filesdir
echo "The number of files are $files_num and the number of matching lines are $matched_lines_num" 

