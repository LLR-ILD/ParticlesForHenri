PROD_NAME=pions50
N_EVENTS=50


run_simulation () {
    if [ -e data/$PROD_NAME ]; then
        printf "$PROD_NAME was already used for another production run. "
        printf "Please remove that run or change PROD_NAME."
    else
        source ./init.sh
        mkdir -p data/$PROD_NAME
        ddsim \
            --steeringFile /cvmfs/ilc.desy.de/sw/ILDConfig/$ILCSOFT_VERSION/StandardConfig/production/ddsim_steer.py \
            --compactFile $lcgeo_DIR/ILD/compact/$ILD_MODEL/$ILD_MODEL.xml \
            --enableGun \
            --gun.particle pi+ \
            --gun.energy 40*GeV \
            --gun.position 0,0,0 \
            --gun.distribution uniform \
            --numberOfEvents=$N_EVENTS \
            --outputFile data/$PROD_NAME/$PROD_NAME.slcio \
        2>&1 | tee data/$PROD_NAME/sim.log
    fi
}

run_lcio2ascii () {
    # Deprecated: This does not work properly, as it 
    source ./init.sh
    load_lcio2ascii
    lcio2ascii data/$PROD_NAME/ascii data/$PROD_NAME/$PROD_NAME.slcio \
        0 0 $(($N_EVENTS - 1)) \
    2>&1 | tee data/$PROD_NAME/convert_lcio2ascii.log
}

run_pylcio_powered_2ascii () {
    source ./init.sh
    # TODO: Wait for input from Henri, saing which collections he actually wants to have.
    # Calls could have a form similar to:
    dumpevent data/$PROD_NAME/$PROD_NAME.slcio 0 $(($N_EVENTS - 1)) \
    2>&1 | tee data/$PROD_NAME/convert_pylcio2ascii.log
}

run_simulation
run_pylcio_powered_2ascii
