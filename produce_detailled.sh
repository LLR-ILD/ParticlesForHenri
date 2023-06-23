ILCSOFT_VERSION=v02-02-02
# Choose one of the detector models from https://github.com/iLCSoft/lcgeo/tree/master/ILD/compact.
ILD_MODEL=ILD_l5_v02
PROD_NAME_PREFIX=v5calo_5GeV_100um
N_EVENTS=50
PROD_NAME=pi+

change_particle_type () {
    if [ "$1" == "" ]; then
        PARTICLE_TYPE=pi+
    else
        PARTICLE_TYPE=$1
    PROD_NAME=${PROD_NAME_PREFIX}/${PARTICLE_TYPE}
    fi
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
            --compactFile $lcgeo_DIR/ILD/compact/$ILD_MODEL/$ILD_MODEL.xml \
            --enableDetailedShowerMode \
            --enableGun \
            --gun.particle $PARTICLE_TYPE \
            --gun.energy 5*GeV \
            --gun.position 0,0,0 \
            --gun.distribution uniform \
            --numberOfEvents=$N_EVENTS \
            --outputFile data/$PROD_NAME/$PARTICLE_TYPE.slcio \
        2>&1 | tee data/$PROD_NAME/sim_${PARTICLE_TYPE}.log
        echo $N_EVENTS "produced in" "data/$PROD_NAME/$PARTICLE_TYPE.slcio"
    fi
    
}

run_pylcio_powered_2ascii () {
    python pylcio_powered_2ascii.py \
        data/$PROD_NAME/$PARTICLE_TYPE.slcio \
        data/$PROD_NAME/py_ascii \
        0 $(($N_EVENTS - 1))
}

source /cvmfs/ilc.desy.de/sw/x86_64_gcc82_centos7/${ILCSOFT_VERSION}/init_ilcsoft.sh

if [ "$1" == "" ]; then
    echo "No particle type specified. All will be created."
    run_simulation
    run_pylcio_powered_2ascii

    change_particle_type pi-
    run_simulation gamma
    run_pylcio_powered_2ascii

    run_simulation gamma
    run_pylcio_powered_2ascii
else
    change_particle_type $1
    run_simulation $2
    run_pylcio_powered_2ascii
fi
