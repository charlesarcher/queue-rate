#!/bin/sh

if [ $# -lt 1 ]
then
    echo "Error in $0 - Invalid Argument Count"
    echo "Syntax: $0 <gnu|intel|pgi>"
    exit
fi

LIBRARY=$2
if [ "$LIBRARY" == "optimized" ]; then
    LIBRARY="optimized"
elif [ "$LIBRARY" == "" ]; then
    LIBRARY="debug"
elif [ "$LIBRARY" == "debug" ]; then
    LIBRARY="debug"
else
    echo "Invalid opt set, use <debug|optimized>"
    exit 1
fi

if [ ! "$3" ];then
    BUILDTAG="base"
else
    BUILDTAG=$3
fi


PWD=$(pwd)
COMPILER=$1
POSTRATEDIR=${PWD}/queue-rate
INSTALL_DIR=/home/cjarcher/code/install/${COMPILER}/queue-rate-${LIBRARY}-${BUILDTAG}
STAGEDIR=${PWD}/stage/${COMPILER}/queue-rate-${LIBRARY}-${BUILDTAG}
export AUTOCONFTOOLS=/home/cjarcher/tools/x86/bin
export PATH=${AUTOCONFTOOLS}:${COMPILERPATH}:/bin:/usr/bin

case $COMPILER in
    pgi )
        export AR=ar
        export LD=ld
        export CC=pgc
        export CXX=pgc++
        export F77=pgf77
        export FC=pgfortran
        ;;
    intel )
        . ${HOME}/intel_compilervars.sh intel64
        . ${HOME}/code/setup_intel.sh
        ;;
    gnu )
        if [ -e /opt/rh/devtoolset-4/enable ]; then
            . /opt/rh/devtoolset-4/enable
        fi
        . ${HOME}/code/setup_gnu.sh
        ;;
    clang )
        if [ -e /opt/rh/devtoolset-4/enable ]; then
            . /opt/rh/devtoolset-4/enable
        fi
        . ${HOME}/code/setup_clang.sh
        ;;
    * )
        echo "Unknown compiler type"
        exit
esac

#hwloc
EXTRA_LD_OPT="-L/home/cjarcher/code/install/gnu/hwloc/lib/ -Wl,-rpath=/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc"
EXTRA_LD_DEBUG="-L/home/cjarcher/code/install/gnu/hwloc/lib/ -Wl,-rpath=/home/cjarcher/code/install/gnu/hwloc/lib -lhwloc"
EXTRA_OPT="${EXTRA_OPT} -I/home/cjarcher/code/install/gnu/hwloc/include"
EXTRA_DEBUG="${EXTRA_DEBUG} -I/home/cjarcher/code/install/gnu/hwloc/include"


OPTFLAGS_COMMON="-Wall -ggdb -O3 -DNDEBUG ${EXTRA_OPT}"
OPTLDFLAGS_COMMON="-O3 ${EXTRA_LD_OPT}"
DEBUGFLAGS_COMMON="-Wall -ggdb -O0 ${EXTRA_DEBUG}"
DEBUGLDFLAGS_COMMON="-O0 ${EXTRA_LD_DEBUG}"

case ${LIBRARY} in
    optimized)
        export CFLAGS=${OPTFLAGS_COMMON}
        export CXXFLAGS=${OPTFLAGS_COMMON}
        export LDFLAGS=${OPTLDFLAGS_COMMON}
        ;;
    debug)
        export CFLAGS=${DEBUGFLAGS_COMMON}
        export CXXFLAGS=${DEBUGFLAGS_COMMON}
        export LDFLAGS=${DEBUGLDFLAGS_COMMON}
        ;;
    *)
        echo "${OPTION}: Unknown optimization type:  use optimized|debug"
        exit 1
        ;;
esac

mkdir -p ${STAGEDIR} && cd ${STAGEDIR}
echo "----------------------------------------------------------------"
echo BUILDTAG:    $BUILDTAG
echo CC:          $CC
echo CFLAGS:      $CFLAGS
echo CXXFLAGS:    $CXXFLAGS
echo LDFLAGS:     $LDFLAGS
echo FLAVOR:      $BUILDFLAVOR
echo STAGEDIR:    $STAGEDIR
echo INSTALL_DIR: $INSTALL_DIR
echo "----------------------------------------------------------------"

if [ ! -f ./Makefile ] ; then                                    \
    echo " ====== BUILDING Post Rate Optimized Library ======="; \
    ${POSTRATEDIR}/configure                                     \
              --prefix=${INSTALL_DIR}
fi && make V=0 -j32 && make install
[ $? -eq 0 ] || exit $?;
