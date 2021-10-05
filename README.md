# ParticlesForHenri

Build human-readable files from events in an `.slcio` file.

The resulting file-format and content was chosen for the needs of Henri.
`produce_pions.sh` shows an example of a full reconstruction chain:

1. Generate events in the ILD detector (in `.slcio` format).
2. From this file, create the human-readable files through `pylcio_powered_2ascii.py`.
