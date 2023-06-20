ILCSOFT_VERSION=v02-02-02

#STDHEP_PATH="/eos/experiment/fcc/ee/generation/stdhep/winter2023/"
#INPUT_FILE="wzp6_ee_qqH_ecm240/events_088604297.stdhep.gz"
#STDHEP_PATH="/eos/experiment/fcc/ee/generation/hepmc/p8_ee_Zud_ecm91/"
#INPUT_FILE="events_noVtxSmear.hepmc"
STDHEP_PATH="./data/CLD/FullSim/"
INPUT_FILE="events_088604297.stdhep"
NEVENTS=10

OUTPUT_PATH=${STDHEP_PATH}
OUTPUT_FILE="./wzp6_ee_qqH_ecm240_fullSim.slcio"

#GEOM_XML="/afs/cern.ch/work/l/lportale/public/fcc_studies/FCCDetectors/Detector/DetFCCeeCLD/compact/FCCee_o2_v02/FCCee_o2_v02.xml"
GEOM_XML="ILD_I5_v02.xml"
STEERING="./ddsim_steer.py"

source /cvmfs/ilc.desy.de/sw/x86_64_gcc82_centos7/${ILCSOFT_VERSION}/init_ilcsoft.sh

ddsim --inputFiles ${STDHEP_PATH}${INPUT_FILE} --outputFile=${OUTPUT_PATH}${OUTPUT_FILE} \
	--compactFile ${GEOM_XML} \
        --numberOfEvents ${NEVENTS} \
	--steeringFile=${STEERING} # > ddsim.out 2>&1 &
