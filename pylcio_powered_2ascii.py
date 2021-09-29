from __future__ import absolute_import
from __future__ import print_function  # python3 style printing in python2.
import logging
import os
import sys

from pyLCIO.io import LcioReader
from pyLCIO import EVENT, IMPL, UTIL
from ROOT import vector

# logging.DEBUG for debugging. Else logging.INFO.
format = "%(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
logging.basicConfig(format=format, level=logging.DEBUG)


help_string = """The script must be called with 2 or 4 arguments as:
$ python pylcio_powered_2ascii.py slcio_file ascii_out_dir ev_start ev_stop
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


def get_hit_contribution_info(hit):
    n_hits_electron = 0
    n_hits_not_electron = 0
    time_earliest_not_electron_if_possible = float("inf")
    pdg_earliest_not_electron_if_possible = None
    for i in range(hit.getNMCContributions()):
        if hit.getPDGCont(i) == 11:
            n_hits_electron += 1
            if n_hits_not_electron == 0:
                contribution_time = hit.getTimeCont(i)
                if contribution_time < time_earliest_not_electron_if_possible:
                    time_earliest_not_electron_if_possible = contribution_time
                    pdg_earliest_not_electron_if_possible = hit.getPDGCont(i)

        else:
            n_hits_not_electron += 1
            contribution_time = hit.getTimeCont(i)
            if contribution_time < time_earliest_not_electron_if_possible:
                time_earliest_not_electron_if_possible = contribution_time
                pdg_earliest_not_electron_if_possible = hit.getPDGCont(i)
    return dict(
        n_not_el=n_hits_not_electron,
        n_el=n_hits_electron,
        first_pdg=pdg_earliest_not_electron_if_possible,
        first_time=time_earliest_not_electron_if_possible,
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
    ]

    def __init__(self, event, ascii_out_dir, event_identifier):
        self.event = event

        lines = []
        for collection_calo in self.collections_SimCalorimeterHit:
            subdetector_lines = self.get_sim_calo_lines(collection_calo)
            lines.extend(subdetector_lines)

        hit_file_name = "{:s}/{:s}.hits".format(ascii_out_dir, event_identifier)
        with open(hit_file_name, "w") as f:
            f.write("\n".join(lines) + "\n")


    def get_sim_calo_lines(self, collection_name):
        calo_hits = self.event.getCollection(collection_name)
        cell_id_encoding = calo_hits.getParameters().getStringVal(EVENT.LCIO.CellIDEncoding)
        self._id_decoder = UTIL.BitField64(cell_id_encoding)
        lines = []
        for hit in calo_hits:
            lines.append(self.get_calo_hit_line(hit))
        return lines


    def get_calo_hit_line(self, hit):
        cell_id_info = ["system", "stave", "module", "cellX", "cellY", "layer"]
        cell_id = (hit.getCellID0() & 0xffffffff) | (hit.getCellID1() << 32)
        self._id_decoder.setValue(cell_id)
        line_entries = dict()
        for cell_id_key in cell_id_info:
            line_entries[cell_id_key] = self._id_decoder[cell_id_key].value()
        line_entries["energy"] = hit.getEnergy()
        line_entries["pos_x"] = hit.getPosition()[0]
        line_entries["pos_y"] = hit.getPosition()[1]
        line_entries["pos_z"] = hit.getPosition()[2]

        line_entries.update(get_hit_contribution_info(hit))
        string_template = " ".join([
            "{system:d} {stave:d} {module:d} {cellX:d} {cellY:d} {layer:d}",
            "{energy:.5e}, {pos_x:.5e}, {pos_y:.5e}, {pos_z:.5e}",
            "{n_not_el:d} {n_el:d} {first_pdg:d} {first_time:.5e}",
        ])
        return string_template.format(**line_entries)


def write_events_to_ascii(slcio_file, ascii_out_dir, ev_start, ev_stop):
    reader = LcioReader.LcioReader(slcio_file)
    if ev_stop < 0:
        ev_stop = reader.getNumberOfEvents() + ev_stop + 1

    for i, event in enumerate(reader):
        if i < ev_start:
            continue
        if i >= ev_stop:
            break
        event_identifier = "{0:04}".format(i)
        Event2Ascii(event, ascii_out_dir, event_identifier)


def validate_command_line_args():
    """This just validates and returns the command line inputs."""
    if len(sys.argv) not in [3, 5]: raise Exception(help_string)

    slcio_file = sys.argv[1]
    if not os.path.isfile(slcio_file): raise Exception(help_string)
    ascii_out_dir = os.path.abspath(sys.argv[2])
    ascii_out_parent = os.path.dirname(ascii_out_dir)
    if not os.path.isdir(ascii_out_parent): raise Exception(help_string)
    if not os.path.exists(ascii_out_dir):
        os.mkdir(ascii_out_dir)
    elif len(os.listdir(ascii_out_dir)) != 0: raise Exception(help_string)

    if len(sys.argv) == 3:
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
    write_events_to_ascii(slcio_file, ascii_out_dir, ev_start, ev_stop)
