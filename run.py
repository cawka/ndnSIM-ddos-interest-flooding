#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from subprocess import call
from sys import argv
import os
import subprocess
import workerpool
import multiprocessing
import argparse

######################################################################
######################################################################
######################################################################

parser = argparse.ArgumentParser(description='Simulation runner')
parser.add_argument('scenarios', metavar='scenario', type=str, nargs='*',
                    help='Scenario to run')

parser.add_argument('-l', '--list', dest="list", action='store_true', default=False,
                    help='Get list of available scenarios')

parser.add_argument('-s', '--simulate', dest="simulate", action='store_true', default=False,
                    help='Run simulation and postprocessing (false by default)')

parser.add_argument('-g', '--no-graph', dest="graph", action='store_false', default=True,
                    help='Do not build a graph for the scenario (builds a graph by default)')

args = parser.parse_args()

if not args.list and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1)

if args.list:
    print "Available scenarios: "
else:
    if args.simulate:
        print "Simulating the following scenarios: " + ",".join (args.scenarios)

    if args.graph:
        print "Building graphs for the following scenarios: " + ",".join (args.scenarios)

######################################################################
######################################################################
######################################################################

class SimulationJob (workerpool.Job):
    "Job to simulate things"
    def __init__ (self, cmdline):
        self.cmdline = cmdline
    def run (self):
        print (" ".join (self.cmdline))
        subprocess.call (self.cmdline)

pool = workerpool.WorkerPool(size = multiprocessing.cpu_count())

class Processor:
    def run (self):
        if args.list:
            print "    " + self.name
            return

        if "all" not in args.scenarios and self.name not in args.scenarios:
            return

        if args.list:
            pass
        else:
            if args.simulate:
                self.simulate ()
                pool.join ()
                self.postprocess ()
            if args.graph:
                self.graph ()

    def graph (self):
        subprocess.call ("./graphs/%s.R" % self.name, shell=True)

class ConvertTopologies (Processor):
    def __init__ (self, name, runs, topologies, buildGraph = False):
        self.name = name
        self.runs = runs
        self.topologies = topologies
        self.buildGraph = buildGraph

        for run in runs:
            try:
                os.mkdir ("topologies/bw-delay-rand-%d" % run)
            except:
                pass # ignore the error

    def simulate (self):
        for topology in self.topologies:
            for run in self.runs:
                cmdline = ["./build/rocketfuel-maps-cch-to-annotaded",
                           "--topology=topologies/rocketfuel_maps_cch/%s.cch" % topology,
                           "--run=%d" % run,
                           "--output=topologies/bw-delay-rand-%d/%s" % (run, topology),
                           "--buildGraph=%d" % self.buildGraph,
                           "--keepLargestComponent=1",
                           "--connectBackbones=1",
                           "--clients=3",
                           ]
                job = SimulationJob (cmdline)
                pool.put (job)

    def postprocess (self):
        # any postprocessing, if any
        pass

    def graph (self):
        pass

try:
    conversion = ConvertTopologies (name="convert-topologies",
                                    runs=[1],
                                    topologies=["1221.r0",
                                                "1239.r0",
                                                "1755.r0",
                                                "2914.r0",
                                                "3257.r0",
                                                "3356.r0",
                                                "3967.r0",
                                                "4755.r0",
                                                "6461.r0",
                                                "7018.r0",],
                                    buildGraph = True)
    conversion.run ()

finally:
    pool.join ()
    pool.shutdown ()
