#!/bin/sh

#set +bm
trap "kill 0" SIGINT

yell()      { echo "$0: $*" >&2; }
yellspace() { echo "     -----> $0: $*" >&2; }
die()       { yell "$*"; exit 111; }
try()       { "$@" || die "cannot $*"; }

killtree() {
    local pid=$1 child
    for child in $(pgrep -P $pid); do
        killtree $child
    done
    if [ $pid -ne $$ ];then
        kill -kill $pid 2> /dev/null
    fi
}



if [ $# -lt 2 ]
then
    echo "Error in $0 - Invalid Argument Count"
    echo "Syntax: $0 <nthreads> <test_bucket>=lock|queue"
    exit
fi

case $2 in
    lock)
        TESTS="lockrate_clh  lockrate_mpsc  lockrate_pthread
               lockrate_pthread_spinlock  lockrate_ticket
               lockrate_tidex  lockrate_tidex_nps"
        ;;
    queue)
        TESTS="qrate_cloudius qrate_folly qrate_mc
               qrate_natsys qrate_vyukov"
        ;;
    alloc)
        TESTS="alloc_rate_boost_malloc   alloc_rate_cloudius_malloc
               alloc_rate_folly_malloc   alloc_rate_mc_malloc
               alloc_rate_natsys_malloc  alloc_rate_tbb_malloc
               alloc_rate_vyukov_malloc  alloc_rate_mc_tbbmalloc
               alloc_rate_vyukov_tbbmalloc"
        TESTS="${TESTS} alloc_rate_boost_li alloc_rate_cloudius_li
               alloc_rate_folly_li alloc_rate_mc_li
               alloc_rate_natsys_li alloc_rate_tbb_li
               alloc_rate_vyukov_li"
        TESTS="${TESTS} alloc_rate_mc_jemalloc alloc_rate_vyukov_jemalloc"
        TESTS="${TESTS} alloc_rate_mc_ssmalloc alloc_rate_vyukov_ssmalloc"

        ;;
    * )
        echo "Unknown test harness type"
        exit
        ;;
esac

for test in $TESTS; do
    if [ ! -f ${test} ]; then
        die "File $test does not exist"
    fi
done

TIMEOUT=10
PWD=$(pwd)
messages=10000000
max_threads=$1
let range=${max_threads}-1
for test in $TESTS; do
    rm -f ${test}.out
    for producers in $(seq 1 $range); do
        consumers=$(expr ${max_threads} - ${producers})
        if [ ${consumers} -le ${producers} ]; then
            let max_producers=$(expr ${max_threads} - ${consumers})
            for allproducers in $(seq ${consumers} ${max_producers}); do
                let total=$(expr ${allproducers} + ${consumers})
                cmd="./$test -p ${allproducers} -c ${consumers} -m ${messages} -r"
                if [ -f ${test} ]; then
                    echo -n "$cmd : "
                    ((eval ${cmd} || die "Error in test" 1>&2) | grep DATAOUT | tee -a ${test}.out) &
                    pid1=$!
                    (sleep ${TIMEOUT}; killtree ${pid1}; echo "KILLED pid ${pid1}";
                     echo "DATAOUT ${allproducers} ${consumers} ${total} -1.0 -1.0" >> ${test}.out ) &
                    pid2=$!
                else
                    die "Fatal:  cannot find file ${test}"
                fi
                wait ${pid1}
                killtree ${pid2} 2>/dev/null
                wait ${pid2} 2>/dev/null
            done
        fi
    done
done
exit
