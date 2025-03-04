#!/usr/bin/python

import os
import sys
import subprocess
import argparse
import shutil
import copy

baseLocation = os.path.dirname(os.path.realpath(__file__))
inputFile = "backingstorecompose_cs.frag"

basename, extension = os.path.splitext(inputFile)

print(f"## {basename} {extension}")


shaderVariants = {
    "srgb_to_scrgb": "SRGB_TO_SCRGB",
    "srgb_to_bt2020pq": "SRGB_TO_BT2020PQ",
    "scrgb_to_srgb": "SCRGB_TO_SRGB",
    "scrgb_to_bt2020pq": "SCRGB_TO_BT2020PQ",
}

for variant, definition in shaderVariants.items():
    commandToRun = ' '.join(['qsb',
                                '--qt6',
                                '-D{}=1'.format(definition),
                                '-o {}_{}{}.qsb'.format(basename, variant, extension),
                                inputFile
                                ])

    # Run the command
    try:
        print( "## RUNNING: " + commandToRun )
        subprocess.check_call( commandToRun, stdout=sys.stdout, stderr=sys.stderr, shell=True,
                                cwd=baseLocation )
    except Exception:
        print(f"## Failed to build a shader \"{variant}\"")
        sys.exit(1)


