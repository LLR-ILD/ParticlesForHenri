ILCSOFT_VERSION=v02-03-03
# Choose one of the detector models from https://github.com/iLCSoft/lcgeo/tree/master/ILD/compact.
ILD_MODEL=ILD_l5_v02_calostep250um
# PROD_NAME_PREFIX=v5calo_250um_90x90deg_1x1deg+5cm  # geometry
#PROD_NAME_PREFIX=v5calo_250um_45x90_1x1deg+5cm  # geometry
PROD_NAME_PREFIX=v5calo_250um_90x22.5_1x1deg+5cm  # geometry
# Defaults
PARTICLE_TYPE=pi+
PARTICLE_ENERGY=5
PARTICLE_MULTIPLICITY=1
N_EVENTS=50
PROD_NAME=${PARTICLE_TYPE}/${PARTICLE_ENERGY}

change_particle_type_energy_mult () {
    if [ "$3" != "" ]; then
        PARTICLE_MULTIPLICITY=$3
    fi
    if [ "$2" != "" ]; then
        PARTICLE_ENERGY=$2
    fi
    if [ "$1" != "" ]; then
        PARTICLE_TYPE=$1
    fi
    if (($PARTICLE_MULTIPLICITY>1)); then M=$PARTICLE_MULTIPLICITY; else M="" ; fi
    PROD_NAME=${PROD_NAME_PREFIX}/${M}${PARTICLE_TYPE}/${PARTICLE_ENERGY}
}

run_simulation () {
    if [ "$1" != "" ]; then
        N_EVENTS=$1
    fi
    if [ -e data/$PROD_NAME ]; then
        printf "$PROD_NAME was already used for another production run. "
        printf "Please remove that run or change PROD_NAME.\n"
    else
        mkdir -p data/$PROD_NAME
        # --compactFile $lcgeo_DIR/ILD/compact/$ILD_MODEL/$ILD_MODEL.xml \
        ddsim \
            --steeringFile /cvmfs/ilc.desy.de/sw/ILDConfig/$ILCSOFT_VERSION/StandardConfig/production/ddsim_steer.py \
            --compactFile $ILD_MODEL.xml \
            --enableGun \
            --gun.particle $PARTICLE_TYPE \
            --gun.energy $PARTICLE_ENERGY*GeV \
            --gun.multiplicity $PARTICLE_MULTIPLICITY \
            --gun.position 0,0,50*mm \
            --gun.thetaMin "89.5*deg" \
            --gun.thetaMax "90.5*deg" \
            --gun.phiMin "22.0*deg" \
            --gun.phiMax "23.0*deg" \
            --gun.distribution "cos(theta)" \
            --numberOfEvents=$N_EVENTS \
            --outputFile data/$PROD_NAME/$PARTICLE_TYPE.slcio \
        2>&1 | tee data/$PROD_NAME/sim_${PARTICLE_TYPE}.log
        echo $N_EVENTS "produced in" "data/$PROD_NAME/$PARTICLE_TYPE.slcio"
    fi
    #             --enableDetailedShowerMode  is to have detaille MC tracks in     
}

run_pylcio_powered_2ascii () {
    python pylcio_powered_2ascii_v3.py \
        data/$PROD_NAME/$PARTICLE_TYPE.slcio \
        data/$PROD_NAME/py_ascii
}

if [ "${ILCSOFT_VERSION}" \< "v02-02-99" ]; then
    source /cvmfs/ilc.desy.de/sw/x86_64_gcc82_centos7/${ILCSOFT_VERSION}/init_ilcsoft.sh
else
    source /cvmfs/ilc.desy.de/sw/x86_64_gcc103_centos7/${ILCSOFT_VERSION}/init_ilcsoft.sh
fi

if [ "$1" == "all" ]; then
    particles=("gamma" "pi+")
    energies=(0.5 1 2 5 10 20 50)
#    multiplicities=(1 2)
    multiplicities=(1)
    echo "No particle type specified. All will be created."
    for p in ${particles[@]}; do
        for e in ${energies[@]}; do
            for m in ${multiplicities[@]}; do
                change_particle_type_energy_mult $p $e $m
                echo $PROD_NAME
                run_simulation
                run_pylcio_powered_2ascii
            done
        done
    done
else
    change_particle_type_energy_mult $1 $2 $3
    run_simulation $4
    run_pylcio_powered_2ascii
fi