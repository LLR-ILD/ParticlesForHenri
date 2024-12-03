from __future__ import absolute_import
from __future__ import print_function  # python3 style printing in python2.
import logging
import os
import sys
import tarfile

from pyLCIO.io import LcioReader
from pyLCIO import EVENT, IMPL, UTIL
from ROOT import vector

# v2 added HCAL hits
# v3 added SET hits

first_evt=True

# logging.DEBUG for debugging. Else logging.INFO.
format = "%(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
logging.basicConfig(format=format, level=logging.DEBUG)

help_string = """The script must be called with 2 or 4 arguments as:
$ python pylcio_powered_2ascii_v2.py slcio_file ascii_out_dir ev_start ev_stop
- slcio_file: File with the reconstructed information in LCIO format.
- ascii_out_dir: Folder to write the ASCII output to.
    Its parent must exist. The folder itself can exist, but must be empty then.
- ev_start ev_stop: Integers. Specify both or neither of them. Will process
    (ev_stop - ev_start) events. Use ev_stop=-1 to exhaust the file.
"""

def get_subdetector_CellIDEncoding(calo_collection):
    """Currently not used."""
    # Key reading taken form: https://github.com/iLCSoft/LCIO/blob/4ec757677231e6da4b9f4c721fe0102cfd53b63e/examples/python/anajob.py#L22
    cell_id_container = vector("string")()
    parameters = calo_collection.getParameters()
    parameters.getStringVals("CellIDEncoding", cell_id_container)
    cell_id_string = cell_id_container[0]
    logging.debug("cell_id_string: " + cell_id_string)
    cell_id_encoding = dict()
    for level_string in cell_id_string.split(","):
        name, value = level_string.split(":", 1)  # maxsplit=1
        cell_id_encoding[name] = value
    logging.debug(cell_id_encoding)
    return cell_id_encoding

def get_calo_hit_contribution_info(self, hit):
    n_hits_electron = 0
    n_hits_not_electron = 0
    time_earliest_not_electron_if_possible = float("inf")
    pdg_earliest_not_electron_if_possible = None
    mcp = None
    for i in range(hit.getNMCContributions()): #suppose time ordering ?
        if hit.getPDGCont(i) == 11:
            n_hits_electron += 1
            if n_hits_not_electron == 0:
                contribution_time = hit.getTimeCont(i)
                if contribution_time < time_earliest_not_electron_if_possible:
                    time_earliest_not_electron_if_possible = contribution_time
                    pdg_earliest_not_electron_if_possible = hit.getPDGCont(i)
                    mcp = self._mcp_ids.get(hit.getParticleCont(i).id())
        else:
            n_hits_not_electron += 1
            contribution_time = hit.getTimeCont(i)
            if contribution_time < time_earliest_not_electron_if_possible:
                time_earliest_not_electron_if_possible = contribution_time
                pdg_earliest_not_electron_if_possible = hit.getPDGCont(i)
                mcp = self._mcp_ids.get(hit.getParticleCont(i).id())

    return dict(
        n_not_el=n_hits_not_electron,
        n_el=n_hits_electron,
        first_pdg=pdg_earliest_not_electron_if_possible,
        first_time=time_earliest_not_electron_if_possible,
        mcp_id=mcp
    )

def get_tracker_hit_contribution_info(self, hit):
    mcpart = hit.getMCParticle()
    pdg= 0
    mcpartidx=0
    if mcpart :
        pdg = mcpart.getPDG()
        mcpartidx = self._mcp_ids.get(mcpart.id())
    time = hit.getTime()
    return dict(
        pdg=pdg,
        time=time,
        mcp=mcpartidx
    )

class Event2Ascii:
    """Event dumping is inspired on LCIO's dumpEvent/dumpEventDetailed:

    https://github.com/iLCSoft/LCIO/blob/8f9e86b93b7d5d83221fabb872ed7e82f1638476/src/cpp/src/UTIL/LCTOOLS.cc#L83
    """
    collections_SimCalorimeterHit = [
        "ECalBarrelSiHitsEven",
        "ECalBarrelSiHitsOdd",
        "ECalEndcapSiHitsEven",
        "ECalEndcapSiHitsOdd",
        "EcalEndcapRingCollection",
        "HCalBarrelRPCHits",
        "HCalECRingRPCHits",
        "HCalEndcapRPCHits"
    ]

    collections_SimTrackerHit = [
        "SETCollection"
    ]

    def __init__(self, event, ascii_out_dir, event_identifier):
        self.event = event

        # put mc part first to refer to them later (tracker hits)
        mc_lines = self.get_mc_lines()
        mc_file_name = "{:s}/event{:s}.nikp".format(ascii_out_dir, event_identifier)
        with open(mc_file_name, "w") as f:
            f.write("\n".join(mc_lines) + "\n")

        hit_lines = []
        subhit_lines = []
        for collection_calo in self.collections_SimCalorimeterHit:
            subdetector_hit_lines, subdetector_subhit_lines = self.get_sim_calo_lines(
                collection_calo, 
                line_number_offset=len(hit_lines),
            )
            hit_lines.extend(subdetector_hit_lines)
            subhit_lines.extend(subdetector_subhit_lines)

        thit_lines = [] # tracker hits
        for collection_tracker in self.collections_SimTrackerHit:
            subdetector_hit_lines = self.get_sim_tracker_lines(
                collection_tracker, 
                line_number_offset=len(thit_lines),
            )
            thit_lines.extend(subdetector_hit_lines)
        print("number of tracker hits lines: ",len(thit_lines))

        hit_file_name = "{:s}/calo{:s}.hits".format(ascii_out_dir, event_identifier)
        with open(hit_file_name, "w") as f:
            f.write("\n".join(hit_lines) + "\n")

        subhit_file_name = "{:s}/calo{:s}.subhits".format(ascii_out_dir, event_identifier)
        with open(subhit_file_name, "w") as f:
            f.write("\n".join(subhit_lines) + "\n")

        thit_file_name = "{:s}/tracker{:s}.hits".format(ascii_out_dir, event_identifier)
        with open(thit_file_name, "w") as f:
            f.write("\n".join(thit_lines) + "\n")

    def get_parent(self, mcp):
        parents = mcp.getParents()
        if len(parents) == 0:
            parent_id = -1
        elif len(parents) > 1:
            raise Exception("No particle should have multiple parents.")
        else:
            parent_id = self._mcp_ids.get(parents[0].id())
        return parent_id

    def get_time_1st_daughter(self, mcp):
        daughters = mcp.getDaughters()
        if len(daughters) == 0:
            time = 0
        else:
            time = daughters[0].getTime()
        return time

    def get_mc_particle_line(self, mcp):
        index = self._mcp_ids.get(mcp.id())
        if index is None:
            index = len(self._mcp_ids)
            self._mcp_ids[mcp.id()] = index
        line_entries = dict(index=index)
        line_entries["pdg"] = mcp.getPDG()
        line_entries["charge"] = mcp.getCharge()
        line_entries["energy"] = mcp.getEnergy()
        line_entries["parent"] = self.get_parent(mcp)
        line_entries["status"] = 0 #mcp.getSimulatorStatus()
        line_entries["prim_rank"] = 0 # mcp.isPrimary()  # count rank to real primary ?
        line_entries["start_t"] = mcp.getTime()
        line_entries["start_x"] = mcp.getVertex()[0]
        line_entries["start_y"] = mcp.getVertex()[1]
        line_entries["start_z"] = mcp.getVertex()[2]
        line_entries["mom_x"] = mcp.getMomentum()[0]
        line_entries["mom_y"] = mcp.getMomentum()[1]
        line_entries["mom_z"] = mcp.getMomentum()[2]
        line_entries["end_t"] = self.get_time_1st_daughter(mcp) # calculated from 1st daugther's vertex
        line_entries["end_x"] = mcp.getEndpoint()[0]
        line_entries["end_y"] = mcp.getEndpoint()[1]
        line_entries["end_z"] = mcp.getEndpoint()[2]

        string_template = " ".join([
            "{index:d} {pdg:d} {parent:d} {status:d} {prim_rank:d} {charge:.1f}",
            "{start_t:.5e} {start_x:.5e} {start_y:.5e} {start_z:.5e}",
            "{energy:.5e} {mom_x:.5e} {mom_y:.5e} {mom_z:.5e}",
            "{end_t:.5e} {end_x:.5e} {end_y:.5e} {end_z:.5e}",
        ])
        return string_template.format(**line_entries)

    def get_mc_lines(self, collection_name="MCParticle"):
        self._mcp_ids = {}
        mc_collection = self.event.getCollection(collection_name)
        lines = []
        for mcp in mc_collection:
            lines.append(self.get_mc_particle_line(mcp))
        return lines

    def get_sim_calo_lines(self, collection_name, line_number_offset=0):
        # print("Collection :", collection_name)
        calo_hits = self.event.getCollection(collection_name)
        is_ecal_central = (collection_name in ["ECalBarrelSiHitsEven", "ECalBarrelSiHitsOdd", "ECalEndcapSiHitsEven", "ECalEndcapSiHitsOdd",
                                               "ECalBarrelScHitsEven", "ECalBarrelScHitsOdd", "ECalEndcapScHitsEven", "ECalEndcapScHitsOdd"])
        cell_id_encoding = calo_hits.getParameters().getStringVal(EVENT.LCIO.CellIDEncoding)
        self._id_decoder = UTIL.BitField64(cell_id_encoding)
        lines = []
        subhit_lines = []
        for i ,hit in enumerate(calo_hits, start=line_number_offset+1):
            lines.append(self.get_calo_hit_line(hit, is_ecal_central))
            subhit_lines.extend(self.get_calo_subhit_lines(hit, i))
        return lines, subhit_lines

    def get_calo_hit_line(self, hit, is_ecal_central=False):
        cell_id_info = ["system", "stave", "module", "x", "y", "tower", "layer", "wafer", "slice"]
        EcalBarrel_replacements = dict(x="cellX", y="cellY")
        # ECal*SiHits*	  : system:0:5,module:5:3,stave:8:4,tower:12:4,layer:16:6,wafer:22:6,slice:28:4,cellX:32:-16,cellY:48:-16
        # Ecal*RingColl   : system:0:5,module:5:3,stave:8:4,tower:12:3,layer:15:6,x:32:-16,y:48:-16
        # HCal*RPCHits    : system:0:5,module:5:3,stave:8:4,tower:12:3,layer:15:6,slice:21:4,x:32:-16,y:48:-16
        # Hcal*Collection : system:0:5,module:5:3,stave:8:4,tower:12:3,layer:15:6,slice:21:4,x:32:-16,y:48:-16
        cell_id = (hit.getCellID0() & 0xffffffff) | (hit.getCellID1() << 32)
        self._id_decoder.setValue(cell_id)
        line_entries = dict()
        for cell_id_key in cell_id_info:
            encoded_key = cell_id_key
            if is_ecal_central:
                if cell_id_key in EcalBarrel_replacements:
                    encoded_key = EcalBarrel_replacements[cell_id_key]
            if cell_id_key in ["wafer", "slice"]:
                # Endcaps have no wafer information.
                line_entries[cell_id_key] = -1
                continue
            # print(cell_id_key, encoded_key)
            line_entries[cell_id_key] = self._id_decoder[encoded_key].value()
        line_entries["energy"] = hit.getEnergy()
        line_entries["pos_x"] = hit.getPosition()[0]
        line_entries["pos_y"] = hit.getPosition()[1]
        line_entries["pos_z"] = hit.getPosition()[2]

        line_entries.update(get_calo_hit_contribution_info(self, hit))
        string_template = " ".join([
            "{system:d} {stave:d} {module:d} {x:d} {y:d} ",
            "{tower:d} {layer:d} {wafer:d}",
            "{energy:.4e} {first_time:.5e} {pos_x:.5e} {pos_y:.5e} {pos_z:.5e}",
            "{first_pdg:d} {mcp_id:d}",
        ])
        return string_template.format(**line_entries)

    def get_calo_subhit_lines(self, hit, hit_id):
        string_template = " ".join([
            "{primary_pdg:d} {secondary_pdg:d} {energy:.4e} {length:.2e}",
            "{time:.5e} {pos_x:.5e} {pos_y:.5e} {pos_z:.5e}",
            "{hit_id:d} {mcp_id:d}",
        ])  
        line_entries = dict(hit_id=hit_id)
        lines = []
        for i_subhit in range(hit.getNMCContributions()):
            line_entries["primary_pdg"] =  hit.getParticleCont(i_subhit).getPDG()
            line_entries["energy"] =  hit.getEnergyCont(i_subhit)
            line_entries["time"] =  hit.getTimeCont(i_subhit)
            line_entries["length"] =  hit.getTimeCont(i_subhit)
            line_entries["length"] = hit.getLengthCont(i_subhit)
            line_entries["secondary_pdg"] = hit.getPDGCont(i_subhit)
            line_entries["pos_x"] = hit.getStepPosition(i_subhit)[0]
            line_entries["pos_y"] = hit.getStepPosition(i_subhit)[1]
            line_entries["pos_z"] = hit.getStepPosition(i_subhit)[2]
            line_entries["mcp_id"] = self._mcp_ids.get(hit.getParticleCont(i_subhit).id())
            lines.append(string_template.format(**line_entries))
        return lines

    def get_sim_tracker_lines(self, collection_name, line_number_offset=0):
        # print("Collection :", collection_name)
        tracker_hits = self.event.getCollection(collection_name)
        cell_id_encoding = tracker_hits.getParameters().getStringVal(EVENT.LCIO.CellIDEncoding)
        self._id_decoder = UTIL.BitField64(cell_id_encoding)
        lines = []
        for i ,hit in enumerate(tracker_hits, start=line_number_offset+1):
            lines.append(self.get_tracker_hit_line(hit))
        return lines

    def get_tracker_hit_line(self, hit):
        cell_id_info = ["system", "side", "layer", "module", "sensor"]
        # SET*colcetoin   : system:0:5,side:5:2,layer:7:9,module:16:8,sensor:24:8
        cell_id = (hit.getCellID0() & 0xffffffff) | (hit.getCellID1() << 32)
        self._id_decoder.setValue(cell_id)
        line_entries = dict()
        for cell_id_key in cell_id_info:
            encoded_key = cell_id_key
            # print(cell_id_key, encoded_key)
            line_entries[cell_id_key] = self._id_decoder[encoded_key].value()
        line_entries["EDep"] = hit.getEDep()
        line_entries["pos_x"] = hit.getPosition()[0]
        line_entries["pos_y"] = hit.getPosition()[1]
        line_entries["pos_z"] = hit.getPosition()[2]

        line_entries.update(get_tracker_hit_contribution_info(self, hit))
        string_template = " ".join([
            "{system:d} {side:d} {layer:d} {module:d} {sensor:d} ",
            "{EDep:.4e} {time:.5e} {pos_x:.5e} {pos_y:.5e} {pos_z:.5e}",
            "{pdg:d} {mcp:d}",
        ])
        return string_template.format(**line_entries)

def write_events_to_ascii(slcio_file, ascii_out_dir, ev_start, ev_stop):
    reader = LcioReader.LcioReader(slcio_file)
    if ev_stop < 0:
        ev_stop = reader.getNumberOfEvents() + ev_stop + 1

    nevent = 0
    for i, event in enumerate(reader):
        if i < ev_start:
            continue
        if i > ev_stop:
            break
        nevent=nevent+1
        event_identifier = "{0:06}".format(i)
        Event2Ascii(event, ascii_out_dir, event_identifier)
    ascii_tar_dir = os.path.join(ascii_out_dir, os.path.pardir)
    ascii_tar_file = os.path.join(ascii_tar_dir, "ascii.tar.gz")
    with tarfile.open(ascii_tar_file, "w:gz") as tar:
        tar.add(ascii_out_dir, arcname=os.path.sep)
    print(nevent, "events printed to ", ascii_out_dir)


def validate_command_line_args():
    """This just validates and returns the command line inputs."""
    if len(sys.argv) not in [2, 5]: raise Exception(help_string)
#    print(len(sys.argv))
    slcio_file = sys.argv[1]
    if not os.path.isfile(slcio_file): raise Exception(help_string)
    if len(sys.argv) == 2:
        ascii_out_dir = os.path.dirname(os.path.normpath(sys.argv[1]))+"/ascii3"
    else:
        ascii_out_dir = os.path.normpath(sys.argv[2])
    ascii_out_parent = os.path.dirname(ascii_out_dir)
    if not os.path.isdir(ascii_out_parent): # check if parent exists
        raise Exception(help_string)
    if not os.path.exists(ascii_out_dir): # create if doesn't exists
        os.mkdir(ascii_out_dir)
    elif len(os.listdir(ascii_out_dir))!=0 : raise Exception(help_string)

    if len(sys.argv) <= 3:
        ev_start = 0
        ev_stop = -1
    else:
        try:
            ev_start = int(sys.argv[3])
            ev_stop = int(sys.argv[4])
        except (IndexError, ValueError): raise Exception(help_string)
    return slcio_file, ascii_out_dir, ev_start, ev_stop

if __name__ == "__main__":
    slcio_file, ascii_out_dir, ev_start, ev_stop = validate_command_line_args()
    print("Converting", slcio_file, "to", ascii_out_dir)
    write_events_to_ascii(slcio_file, ascii_out_dir, ev_start, ev_stop)
    print("Convertion done")
