#!/bin/bash

a=1
for i in ./0/*; do
    new=$(printf "./0/doc%d" "$a")
    mv -- "$i" "$new"
    let a=a+1
done
