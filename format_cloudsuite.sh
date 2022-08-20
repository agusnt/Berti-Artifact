#!/bin/bash

for i in $1/*; do
    sed -i '/UNIQUE/d' $i
done
