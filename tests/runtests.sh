#!/bin/sh

BASEDIR=$(dirname $0)
PWD=`(cd $BASEDIR && pwd)`

if [[ ${PWD} == /opt/* ]]; then
    for testcase in `ls $BASEDIR/tst_*`; do
        ${testcase}
        echo "RET CODE: $?"
        sleep 2
    done
else
    for casedir in `ls -d $BASEDIR/tst_*`; do
        casename=$(basename ${casedir})
        cd (${casedir} && ./${casename})
        echo "RET CODE: $?"
        sleep 2
    done
fi
