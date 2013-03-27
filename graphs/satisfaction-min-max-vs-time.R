#!/usr/bin/env Rscript

args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("ERROR: Scenario parameters should be specified\n")
  q(status = 1)
}

prefixes = args[1]
topo     = args[2]
evils    = args[3]
runs     = args[4]
folder   = args[5]
good     = args[6]
producer = args[7]

suppressPackageStartupMessages (library(ggplot2))
suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))
suppressPackageStartupMessages (library(plyr))
suppressPackageStartupMessages (library(scales))

source ("graphs/graph-style.R")

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

data.all$Evil = factor(data.all$Evil)

name2 = paste (sep="-", topo, "good", good, "producer", producer)

data.all$Scenario = ordered (data.all$Scenario,
  c("fairness", "satisfaction-accept", "satisfaction-pushback"))

levels(data.all$Scenario) <- sub("^satisfaction-pushback$", "Satisfaction-based pushback", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^satisfaction-accept$",   "Satisfaction-based Interest acceptance", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^fairness$",              "Token bucket with per interface fairness", levels(data.all$Scenario))

cat (sep="", "Writing to ", paste(sep="","graphs/pdfs/", folder, "/",name2,".pdf"))
pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,".pdf"), width=5, height=4)

minTime = 300
attackTime = 300

gdata = subset(data.all, minTime-40 <= Time & Time < minTime+attackTime+100)

g <- ggplot (gdata) +
  stat_summary(aes(x=Time-minTime, y=Ratio, color=Scenario), geom="line", fun.y=mean, size=0.4) +

  stat_summary(aes(x=Time-minTime, y=Ratio, color=Scenario, fill=Scenario),
               geom="errorbar",
               fun.y=mean,
               fun.ymin=min,
               fun.ymax=function(x) {
                 min (1, max(x)) # don't pretend that we can do very good
               },
               data = gdata[sample(nrow(gdata), length(gdata$Time)/20),],
               size=0.1, width=1, alpha=0.5) +
  
  theme_custom () +
  xlab ("Time since attack started, seconds") +
  ylab ("Min/max satisfaction ratios") +
  ## ggtitle ("Satisfaction ratio dynamics during the attack (~30% attackers)") +
  scale_colour_brewer(palette="Set1") +
  scale_fill_brewer(palette="PuOr") +
  scale_y_continuous (limits=c(0,1), breaks=seq(0,1,0.25), labels=percent_format ()) +
  facet_wrap (~ Scenario, nrow=5, ncol=1) +
  geom_vline(xintercept = attackTime) +
  theme (legend.key.size = unit(0.8, "lines"),
         legend.position="none", #c(1.0, 0.0),
         legend.justification=c(1,0),
         legend.background = element_rect (fill="white", colour="black", size=0.1))  

print (g)

x = dev.off ()
