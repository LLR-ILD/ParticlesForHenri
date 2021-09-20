# Choose one of the detector models from https://github.com/iLCSoft/lcgeo/tree/master/ILD/compact.
ILD_MODEL=ILD_l5_v02
ILCSOFT_VERSION=v02-02-02

build_lcio2ascii () {
    export LCIO2ASCIIHOME=${PWD}/LCIO2ASCII
    export LCIOBASE=${LCIO}
    export LD_LIBRARY_PATH="$LCIO/lib:$LD_LIBRARY_PATH"
    #export PATH="$LCIO/tools:$LCIO/bin:$PATH"
    printf "lcio2ascii will be built now at ${LCIO2ASCIIHOME}.\n"

    echo $LCIO

    cd ${LCIO2ASCIIHOME}
    mkdir -p bin
    mkdir -p build
    cd build
    cmake -D LCIO_DIR=$LCIO  ..
    make -j 4
    cd ../..
}

load_lcio2ascii () {
    if ! [ -e $PWD/LCIO2ASCII/build/lcio2ascii ]; then
        build_lcio2ascii
    fi
    export PATH=$PWD/LCIO2ASCII/build/:$PATH
}

source /cvmfs/ilc.desy.de/sw/x86_64_gcc82_centos7/${ILCSOFT_VERSION}/init_ilcsoft.sh

