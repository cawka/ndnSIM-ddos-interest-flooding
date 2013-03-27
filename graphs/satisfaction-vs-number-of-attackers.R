#!/usr/bin/env Rscript

args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("ERROR: Scenario parameters should be specified\n")
  q(status = 1)
}

suppressPackageStartupMessages (library(ggplot2))
suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))
suppressPackageStartupMessages (library(plyr))
suppressPackageStartupMessages (library(scales))

source ("graphs/graph-style.R")

prefixes = args[1]
topo     = args[2]
evils    = args[3]
runs     = args[4]
folder   = args[5]
good     = args[6]
producer = args[7]

minTime = 300
attackTime = 300

name = paste (sep="-", prefixes, "topo", topo, "evil", evils, "producer", producer)
filename = paste(sep="", "results/",folder,"/process/", name, "-all-data.dat")

if (file_test("-f", filename)) {
  cat ("Loading data from", filename, "\n")
  load (filename)
  
} else {
   data.all = data.frame ()
   for (evil in strsplit(evils,",")[[1]]) {
     for (prefix in strsplit(prefixes,",")[[1]]) {
       name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)
       filename = paste(sep="", "results/",folder,"/process/", name, ".txt")
       cat ("Reading from", filename, "\n")
       ## data = read.table (filename, header=TRUE)
       load (filename)
       
       data.all <- rbind (data.all, data)
     }
   }

   name = paste (sep="-", prefixes, "topo", topo, "evil", evils, "producer", producer)
   filename = paste(sep="", "results/",folder,"/process/", name, "-all-data.dat")
   
   cat ("Saving data to", filename, "\n")
   save (data.all, file=filename)
}

data.all = subset(data.all)

data.all$Evil = factor(data.all$Evil)

name2 = paste (sep="-", topo, "good", good, "producer", producer)

data.all$Scenario = ordered (data.all$Scenario,
  c("fairness", "satisfaction-accept", "satisfaction-pushback"))

levels(data.all$Scenario) <- sub("^satisfaction-pushback$", "Satisfaction-based pushback", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^satisfaction-accept$",   "Satisfaction-based Interest acceptance", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^fairness$",              "Token bucket with per interface fairness", levels(data.all$Scenario))

##########################################################
##########################################################
##########################################################

pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"-avg-1-min.pdf"), width=8, height=5)

satisf.avg = summaryBy (Ratio ~ Node + Scenario + Evil + Run, data=subset(data.all, Time >= minTime & Time < minTime+60), FUN=c(mean))

g <- ggplot (satisf.avg) + 
  geom_boxplot (aes(x=Evil, y=Ratio.mean, fill=Scenario),
                size=0.1, outlier.size=0.5,
                position="dodge") +
  theme_custom () +
  xlab ("Number of attackers / Number of legitimate users") +
  ylab ("Interest satisfaction ratio") +
  scale_fill_brewer(palette="Set1") +
  scale_colour_brewer(palette="PuOr") +
  scale_y_continuous (labels=percent_format (), limits=c(0,1.0)) +
  scale_x_discrete (labels=c("1 / 15", "3 / 13", "5 / 11", "7 / 9", "9 / 7")) +
  ## ggtitle ("Average consumer Interest satisfaction ratios (first minute)") +
  theme (legend.key.height = unit(2.1, "lines"), legend.key.width = unit(0.8, "lines"))  
  ## theme (legend.key.size = unit(0.8, "lines"),
  ##        legend.position=c(1, 0.35),
  ##        legend.justification=c(1,0),
  ##        legend.background = element_rect (fill="white", colour="black", size=0.1))  

print(g)

x = dev.off ()

##########################################################
##########################################################
##########################################################

pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"-avg-1-min-after-1-min.pdf"), width=8, height=5)

satisf.avg = summaryBy (Ratio ~ Node + Scenario + Evil + Run, data=subset(data.all, Time >= minTime+60 & Time < minTime+60+60), FUN=c(mean))

g <- ggplot (satisf.avg) + 
  ## geom_violin (aes(x=Evil, y=Ratio.mean, fill=Scenario), trim=TRUE, adjust=0.8, size=0.3, width=1.1, outlier.size=0.4) +
                  ## trim=TRUE, adjust=0.8, width=0.2, 
  geom_boxplot (aes(x=Evil, y=Ratio.mean, fill=Scenario),
                size=0.1, outlier.size=0.5,
                position="dodge") +
  theme_custom () +
  xlab ("Number of attackers / Number of legitimate users") +
  ylab ("Interest satisfaction ratio") +
  scale_fill_brewer(palette="Set1") +
  scale_colour_brewer(palette="PuOr") +
  scale_x_discrete (labels=c("1 / 15", "3 / 13", "5 / 11", "7 / 9", "9 / 7")) +
  scale_y_continuous (labels=percent_format (), limits=c(0,1.0)) +
  ## ggtitle ("Average consumer Interest satisfaction ratios (second minute)") +
  theme (legend.key.height = unit(2.1, "lines"), legend.key.width = unit(0.8, "lines"))  
  ## theme (legend.key.size = unit(0.8, "lines"),
  ##        legend.position=c(1, 0.35),
  ##        legend.justification=c(1,0),
  ##        legend.background = element_rect (fill="white", colour="black", size=0.1))  

print(g)

x = dev.off ()
