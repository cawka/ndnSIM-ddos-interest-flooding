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

parser.add_argument('-p', '--postprocess', dest="postprocess", action='store_true', default=False,
                    help='Run postprocessing (false by default, always true if simulate is enabled)')

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
            if args.simulate or args.postprocess:
                self.postprocess ()
                pool.join ()
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

class InterestDdosAttack (Processor):
    def __init__ (self, name, algorithms, topologies, evils, good, runs, folder, producer="gw", defaultRtt="250ms"):
        self.name = name
        self.algorithms = algorithms
        self.topologies = topologies
        self.evils = evils
        self.good = good
        self.runs = runs
        self.folder = folder
        self.producer = producer
        self.defaultRtt = defaultRtt

    def simulate (self):
        for algorithm in self.algorithms:
            for topology in self.topologies:
                for evil in self.evils:
                    for run in self.runs:
                        cmdline = ["./build/interest-ddos-attack-and-mitigation-scenario",
                                   "--algorithm=%s" % algorithm,
                                   "--run=%d" % run,
                                   "--topology=%s" % topology,
                                   "--badCount=%d" % evil,
                                   "--goodCount=%d" % self.good,
                                   "--folder=%s" % self.folder,
                                   "--producer=%s" % self.producer,
                                   "--defaultRtt=%s" % self.defaultRtt,
                                   ]
                        job = SimulationJob (cmdline)
                        pool.put (job)

    def postprocess (self):
        print "Convert data to sqlite and reduce data size"

        for algorithm in self.algorithms:
            for topology in self.topologies:
                for evil in self.evils:
                    for run in self.runs:

                        name = "results/%s/%s-topo-%s-evil-%d-good-%d-producer-%s-run-%d" % (
                            self.folder,
                            algorithm,
                            topology,
                            evil,
                            self.good,
                            self.producer,
                            run)
                        print "Converting %s" % name

                        bzip = "%s.txt.bz2" % name
                        sqlite = "%s.db" % name

                        subprocess.call ("rm -f \"%s\"" % sqlite, shell=True)
                        cmd = "bzcat \"%s\" | tail -n +2 | awk -F\"\t\" '{if (NF == 9) {print $1\"|\"$2\"|\"$3\"|\"$5\"|\"$6\"|\"$7}}' | sqlite3 -cmd \"create table data_tmp (Time int, Node text, FaceId int, Type text, Packets real, Kilobytes real)\" -cmd \".import /dev/stdin data_tmp\" \"%s\"" % (bzip, sqlite)
                        subprocess.call (cmd, shell=True)

                        cmd = "sqlite3 -cmd \"create table data as select Time,Node,Type,sum(Packets) as Packets, sum(Kilobytes) as Kilobytes from data_tmp where Type in ('InData', 'OutInterests') group by Time,Node,Type\" -cmd \"drop table data_tmp\" -cmd \"vacuum\" \"%s\" </dev/null"  % sqlite
                        # print cmd
                        subprocess.call (cmd, shell=True)

    def graph (self):
        print "graph_rate_verification"

        for algorithm in self.algorithms:
            for topology in self.topologies:
                for evil in self.evils:
                    # print "Scenario [%s], topology [%s], evils [%d], run [%d]" % (prefix, topology, evil, run)
                    cmdline = ["./graphs/rates.R",
                               algorithm,
                               topology,
                               str(evil),
                               ",".join([str(run) for run in self.runs]),
                               self.folder,
                               str(self.good),
                               self.producer,
                               ]
                    job = SimulationJob (cmdline)
                    pool.put (job)

class SatisfactionMinManFigures (InterestDdosAttack):
    def __init__ (self, name, scenario):
        self.name = name
        self.algorithms = scenario.algorithms
        self.topologies = scenario.topologies
        self.evils = scenario.evils
        self.good = scenario.good
        self.runs = scenario.runs
        self.folder = scenario.folder
        self.producer = scenario.producer
        self.defaultRtt = scenario.defaultRtt

    def simulate (self):
        # Here we just assume that the relevant simulation data is already exists
        pass

    def postprocess (self):
        print "Aggregate data"

        for algorithm in self.algorithms:
            for topology in self.topologies:
                for evil in self.evils:
                    cmdline = ["./graphs/preprocess-satisfaction.R",
                               algorithm,
                               topology,
                               str(evil),
                               ",".join([str(run) for run in self.runs]),
                               self.folder,
                               str(self.good),
                               self.producer,
                               ]
                    job = SimulationJob (cmdline)
                    print ">>> "+" ".join (cmdline)+"<<< \n";
                    pool.put (job)

    def graph (self):
        print "SatisfactionMinManFigures"

        for topology in self.topologies:
            cmdline = ["./graphs/satisfaction-min-max-vs-time.R",
                       ",".join(self.algorithms),
                       topology,
                       ",".join([str(evil) for evil in self.evils]),
                       ",".join([str(run) for run in self.runs]),
                       self.folder,
                       str(self.good),
                       self.producer
                       ]
            job = SimulationJob (cmdline)
            pool.put (job)

class SatisfactionNumberOfAttackersFigures (SatisfactionMinManFigures):
    def simulate (self):
        # Here we just assume that the relevant simulation data is already exists
        pass

    def postprocess (self):
        pass

    def graph (self):
        print "SatisfactionNumberOfAttackersFigures"

        for topology in self.topologies:
            cmdline = ["./graphs/satisfaction-vs-number-of-attackers.R",
                       ",".join(self.algorithms),
                       topology,
                       ",".join([str(evil) for evil in self.evils]),
                       ",".join([str(run) for run in self.runs]),
                       self.folder,
                       str(self.good),
                       self.producer
                       ]
            job = SimulationJob (cmdline)
            pool.put (job)

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
    # conversion.run ()

    smallTree = InterestDdosAttack (name="attack-small-tree",
                                    algorithms = ["fairness", "satisfaction-accept", "satisfaction-pushback"],
                                    topologies = ["small-tree"],
                                    evils = [1,2],
                                    good  = 0, # number of client nodes minus number of evil nodes
                                    runs = range(1,11), 
                                    folder = "attackSmallTree",
                                    producer = "gw",
                                    defaultRtt = "80ms")
    smallTree.run ()

    tree = InterestDdosAttack (name="attack-tree",
                               algorithms = ["fairness", "satisfaction-accept", "satisfaction-pushback"],
                               topologies = ["tree"],
                               evils = range(1,10,2),
                               good  = 0, # number of client nodes minus number of evil nodes
                               runs = range(1,11), 
                               folder = "attackTree",
                               producer = "gw",
                               defaultRtt = "80ms")
    tree.run ()

    isp = InterestDdosAttack (name="attack-isp",
                              algorithms = ["fairness", "satisfaction-accept", "satisfaction-pushback"],
                              topologies = ["7018.r0"],
                              evils = [140],
                              good  = 0, # number of client nodes minus number of evil nodes
                              runs = range(1,11), 
                              folder = "attackISP",
                              producer = "gw",
                              defaultRtt = "330ms")
    isp.run ()

    SatisfactionMinManFigures (name="figureX", scenario=smallTree).run ()
    SatisfactionMinManFigures (name="figure6", scenario=tree).run ()
    SatisfactionMinManFigures (name="figure9", scenario=isp).run ()

    SatisfactionNumberOfAttackersFigures (name="figureXX", scenario=smallTree).run ()
    SatisfactionNumberOfAttackersFigures (name="figure7",  scenario=tree).run ()
    # SatisfactionNumberOfAttackersFigures (name="figureYY", scenario=isp).run ()

finally:
    pool.join ()
    pool.shutdown ()
