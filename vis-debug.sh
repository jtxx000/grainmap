#!/bin/bash

{
  echo digraph 'G {'
  sed -nr 's/add edge 0(x[^ ]+) -> 0(x[^ ]+)/\1 -> \2;/p' | sort | uniq
  echo '}'
} | dot -Tpng -o bin/debug.png

#sed -nr 's/traverse region point ([0-9]+) ([0-9]+)/\1,\2/p' $1
