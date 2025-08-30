#! /bin/bash

Input=$1
Output=$2
Filters=$3
widths=$4
imagePath=$5
FontPath=$6
$JAVA_HOME/bin/java -Dfile.encoding=UTF8 -cp "$5/lib/pdf_exporter/"iText-5.0.1.jar:"$5/lib/pdf_exporter/"RecordViewPdfWriter.jar  RecordViewPdfWriter $1 $2 "$3" "$4" "$5" "$6" >>/tmp/pdf.txt 2>&1
