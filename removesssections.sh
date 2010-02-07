#!/bin/bash

for i in latex/*_8h*.tex
do
    ed $i <<EOF_ED
/subsubsection/,$ d
w
q
EOF_ED
done
