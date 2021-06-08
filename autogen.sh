#!/bin/sh
#
# Autotools boostrapping script
#
giveup()
{
        echo
        echo "  Something went wrong, giving up!"
        echo
        exit 1
}

echo "Running autoconf"
autoconf || giveup

echo "======================================"
echo "Now you are ready to run './configure'"
echo "======================================"
