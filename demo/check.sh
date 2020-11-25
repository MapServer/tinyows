#!/bin/sh

set -eu

RET=0

for i in demo/tests/input/*; do
    echo "Running $i"
    QUERY_STRING="$(cat $i)" ./tinyows > /tmp/output.txt
    if ! diff -u /tmp/output.txt demo/tests/expected/$(basename $i); then
        mkdir -p demo/tests/got
        mv /tmp/output.txt demo/tests/got/$(basename $i)
        echo "Result got put in demo/tests/got/$(basename $i)"
        RET=1
    fi
done

if test "$RET" -eq "0"; then
    echo "Tests OK !"
else
    echo "Tests failed !"
fi

exit $RET