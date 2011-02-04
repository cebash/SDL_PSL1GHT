#!/bin/sh
#
# Print the current source revision, if available

# FIXME: this prints the tip, which isn't useful if you're on a different
#  branch, or just not sync'd to the tip.
hg tip --template 'hg-{rev}:{node|short}' ||  git show | head -n1 | sed 's/commit //' || (echo "hg-0:baadf00d"; exit 1)
