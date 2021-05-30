#!/bin/bash
# shellcheck disable=SC2034
for var in $(ps aux | grep mohican | awk '{print $2}')
do
kill $var
done
rm access.log
rm error.log