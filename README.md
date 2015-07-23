Prerequisites
=============

Custom version of NS-3 and specified version of ndnSIM needs to be installed.

The code should also work with the latest version of ndnSIM, but it is not guaranteed.

    mkdir ns-dev
    cd ns-dev

    git clone git://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
    (cd ns-3 && git checkout -b ndnSIM-v1.0 bb96dc1e3ba9d940ab1aae44224fa2b8b45e1a12)
    git clone git://github.com/cawka/pybindgen.git pybindgen
    git clone git://github.com/NDN-Routing/ndnSIM.git ns-3/src/ndnSIM
    (cd ns-3/src/ndnSIM && git checkout -b v0.5-rc2 db5f3b6acf526918be0a6a67c11378fa32cb27de)

    git clone git://github.com/cawka/ndnSIM-ddos-interest-flooding.git ndnSIM-ddos-interest-flooding

    cd ns-3
    ./waf configure -d optimized
    ./waf
    sudo ./waf install

    cd ../ndnSIM-ddos-interest-flooding

After which you can proceed to compile and run the code

For more information how to install NS-3 and ndnSIM, please refer to http://ndnsim.net website.

Compiling
=========

To configure in optimized mode without logging **(default)**:

    ./waf configure

To configure in optimized mode with scenario logging enabled (logging in NS-3 and ndnSIM modules will still be disabled,
but you can see output from ``NS_LOG*`` calls from your scenarios and extensions):

    ./waf configure --logging

To configure in debug mode with all logging enabled

    ./waf configure --debug

If you have installed NS-3 in a non-standard location, you may need to set up ``PKG_CONFIG_PATH`` variable.
For example, if NS-3 is installed in /usr/local/, then the following command should be used to
configure scenario

    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure

or

    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure --logging

or

    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig ./waf configure --debug

Running
=======

Normally, you can run scenarios either directly

    ./build/<scenario_name>

or using waf

    ./waf --run <scenario_name>

If NS-3 is installed in a non-standard location, on some platforms (e.g., Linux) you need to specify ``LD_LIBRARY_PATH`` variable:

    LD_LIBRARY_PATH=/usr/local/lib ./build/<scenario_name>

or

    LD_LIBRARY_PATH=/usr/local/lib ./waf --run <scenario_name>

To run scenario using debugger, use the following command:

    gdb --args ./build/<scenario_name>


Running with visualizer
-----------------------

There are several tricks to run scenarios in visualizer.  Before you can do it, you need to set up environment variables for python to find visualizer module.  The easiest way to do it using the following commands:

    cd ns-dev/ns-3
    ./waf shell

After these command, you will have complete environment to run the vizualizer.

The following will run scenario with visualizer:

    ./waf --run <scenario_name> --vis

or

    PKG_LIBRARY_PATH=/usr/local/lib ./waf --run <scenario_name> --vis

If you want to request automatic node placement, set up additional environment variable:

    NS_VIS_ASSIGN=1 ./waf --run <scenario_name> --vis

or

    PKG_LIBRARY_PATH=/usr/local/lib NS_VIS_ASSIGN=1 ./waf --run <scenario_name> --vis

Available simulations
=====================

Topology converter
------------------

To convert topologies from RocketFuel format and assign random bandwidths and delays for links, you can run the following:

    ./run.py -s convert-topologies

You can edit ``run.py`` script and ``scenarios/rocketfuel-maps-cch-to-annotaded.cc`` to modifiy topology conversion logic
(e.g., you may want to assign different bandwidth range for "backbone-to-backbone" links).

`./run.py` script uses `workerpool` python module, which can be installed on Ubuntu using `easy_install` command:

    sudo easy_install workerpool

Or on Mac that uses `macports`:

    sudo port install py27-workerpool

If `easy_install` is not yet installed, you can install it using the following command:

    sudo apt-get install python-setuptools

For more information about Rocketfuel topology files, please refer to http://www.cs.washington.edu/research/networking/rocketfuel/

You would also need `sqlite3` installed:

On Unbuntu:

    sudo apt-get install sqlite3

On Mac with macports:

    sudo port install sqlite3

Interest flooding attack and attack mitigation mechanisms
---------------------------------------------------------

### Two-level binary tree topology

To perform 10 runs of the simulation for different Interest Flooding mitigation algorithms for 2-level binary tree topology with different number of attackers (1 or 2), use the following:

    ./run.py -s attack-small-tree

After simulation finishes (it can take a couple of hours), the script will build a set of graphs in `graphs/pdf/attackSmallTree` that show Interest and Data rates passing through each simulation node.

To build additional graphs (min-max satisfaction ratios versus time and satisfaction ratios versus number of the attackers), you can run the following:

    ./run.py -p figureX

    ./run.py figureXX

Note that graph building scripts require R package (http://www.r-project.org/) to be properly installed and configured on the system.  If you have installed R, but it gives you error, you may need to manually install missing packages (refer to R documentation).

### Four-level binary tree topology

The following commands will run simulation with four-level binary tree topology (can take several hours to finish):

    ./run.py -s attack-tree

Additional graphs (Figure 6 and Figure 7 from the "Interest Flooding Attack and Countermeasures in Named Data Networking" paper by Afanasyev et al.) can be build using:

    ./run.py -p figure6

    ./run.py figure7

### ISP-like topology

The following commands will run simulation with ISP-like topology (can take many hours to finish):

    ./run.py -s attack-isp

Additional graphs (Figure 9 from the paper) can be build using:

    ./run.py -p figure9

**Note that provided scripts rely on R version at least 2.15 (http://www.r-project.org/) with sqldf, proto, reshape2, ggplot2, scales, plyr, and doBy modules to be installed.**  For example, after you install R, run the following to install necessary modules:

    sudo R
    install.packages ('sqldf', dep=TRUE)
    install.packages ('proto')
    install.packages ('ggplot2', dep=TRUE)
    install.packages ('doBy', dep=TRUE)
