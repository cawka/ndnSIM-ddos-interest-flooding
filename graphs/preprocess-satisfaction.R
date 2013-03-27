#!/usr/bin/env Rscript

args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("ERROR: Scenario parameters should be specified\n")
  q(status = 1)
}

options(gsubfn.engine = "R")
suppressPackageStartupMessages (library(sqldf))
## suppressPackageStartupMessages (library(ggplot2))
## suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))

prefix = args[1]
topo   = args[2]
evil   = args[3]
runs   = args[4]
folder = args[5]
if (is.na (folder)) {
  folder = "newrun"
}
good   = args[6]
producer = args[7]

name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)

data.all = data.frame ()

for (run in strsplit(runs,",")[[1]]) {  
  filename <- paste (sep="", "results/", folder, "/", name, "-run-", run)
  cat ("Reading from", filename, "\n")

  input <- paste (sep="", filename, ".db")
  data.run <- sqldf("select * from data", dbname = input, stringsAsFactors=TRUE)

  nodes.good     = levels(data.run$Node)[ grep ("^good-",     levels(data.run$Node)) ]
  nodes.evil     = levels(data.run$Node)[ grep ("^evil-",     levels(data.run$Node)) ]
  nodes.producer = levels(data.run$Node)[ grep ("^producer-", levels(data.run$Node)) ]

  run.ratios <- function (data) {
    data.interests = subset (data, Node %in% nodes.good & Type == "OutInterests")
    data.interests$Type <- NULL
    data.interests$Kilobytes <- NULL
    
    names(data.interests)[names(data.interests) == "Packets"] = "OutInterests"
    
    data.data      = subset (data, Node %in% nodes.good & Type == "InData")
    data.data$Type <- NULL
    data.data$Kilobytes <- NULL
    names(data.data)[names(data.data) == "Packets"] = "InData"
    
    data.out = merge (data.interests, data.data)
    data.out$Run  = run
    data.out$Ratio = data.out$InData / data.out$OutInterests

    data.out
  }

  data.all = rbind (data.all, run.ratios (data.run))
}

## data.all$Type = factor(data.all$Type)
data.all$Run  = factor(data.all$Run)
data.all$Scenario = factor(prefix)
data.all$Evil     = factor(evil)


outputfile = paste(sep="", "results/",folder,"/process/", name, ".txt")
dir.create (paste(sep="", "results/",folder,"/process/"), showWarnings = FALSE)

cat (">> Writing", outputfile, "\n")
## write.table(data.all, file = outputfile, row.names=FALSE, col.names=TRUE)
data = data.all
save (data, file=outputfile)

