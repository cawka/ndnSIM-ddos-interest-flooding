#!/usr/bin/env Rscript

args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("ERROR: Scenario parameters should be specified\n")
  q(status = 1)
}

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

options(gsubfn.engine = "R")
suppressPackageStartupMessages (library(sqldf))
suppressPackageStartupMessages (library(ggplot2))
suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))


source ("graphs/graph-style.R")

name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)
dir.create (paste(sep="", "graphs/pdfs/", folder, "/"), showWarnings = FALSE, recursive = TRUE)
graphfile = paste(sep="","graphs/pdfs/", folder, "/", name,".pdf")

cat ("Writing to", graphfile, "\n")
pdf (graphfile, width=10, height=7.5)


for (run in strsplit(runs,",")[[1]]) {  
  grid.newpage()
  filename <- paste (sep="", "results/", folder, "/", name, "-run-", run)
  cat ("Reading from", filename, "\n")

  input <- paste (sep="", filename, ".db")
  data.allFaces <- sqldf("select * from data", dbname = input, stringsAsFactors=TRUE)
  
  ## data$Node <- factor(data$Node)
  ## data$FaceId <- factor(data$FaceId)
  data.allFaces$Kilobits <- data.allFaces$Kilobytes * 8
  
  ## data.allFaces = summaryBy (. ~ Time + Node + Type, data=data, FUN=sum)

  xmin = min(data.allFaces$Time)
  xmax = max(data.allFaces$Time)
  
  graph <- function (data, variable='Kilobits', legend="none") {
    data <- subset(data, Type %in% c("InData", "OutInterests")) 
    g <- ggplot (data,
                 aes(x=Time))
    g <- g + geom_point (aes_string(y=variable, color="Type"), size=1)
    ## g <- g + geom_point (aes(y=Packets.sum, color=Type), size=1)
    ## g <- g + scale_y_continuous (trans="log", labels = round)
    g <- g + scale_color_manual (values=c("darkgreen", "red"))
    g <- g + facet_wrap (~ Node, nrow=1)
    g <- g + ggtitle ("Bandwidths")
    g <- g + theme_custom ()
    g <- g + theme(legend.position=legend)
    g <- g + scale_x_continuous (limits=c(xmin, xmax))
    ## g <- g + coord_cartesian(ylim=c(-10,1010))
  }
  
  ## data.good = subset ()
  nodes.good = c(levels(data.allFaces$Node)[ grep ("^good-", levels(data.allFaces$Node)) ])
  nodes.good = sample (nodes.good, min(length(nodes.good), 10))
  ## nodes.good = c("good-leaf-12923")
  
  nodes.evil = levels(data.allFaces$Node)[ grep ("^evil-", levels(data.allFaces$Node)) ]
  nodes.evil = sample (nodes.evil, min(length(nodes.evil), 10))

  nodes.producer = levels(data.allFaces$Node)[ grep ("^producer-", levels(data.allFaces$Node)) ]

  g1 <- graph (subset (data.allFaces, Node %in% nodes.good),     "Kilobits")
  g2 <- graph (subset (data.allFaces, Node %in% nodes.evil),     "Kilobits")
  g3 <- graph (subset (data.allFaces, Node %in% nodes.producer), "Kilobits")
  
  pushViewport(vpList(
    viewport(x = 0.5, y = .66, width = 1, height = .33,
      just = c("center", "bottom"), name = "p1"),
    viewport(x = 0.5, y = .33, width = 1, height = .33,
      just = c("center", "bottom"), name = "p2"),
    viewport(x = 0.5, y = .00, width = 1, height = .33,
      just = c("center", "bottom"), name = "p3")
    ))
  
  ## Add the plots from ggplot2
  upViewport()
  downViewport("p1")
  print(g1, newpage = FALSE)
  
  upViewport()
  downViewport("p2")
  print(g2, newpage = FALSE)
  
  upViewport()
  downViewport("p3")
  print(g3, newpage = FALSE)
  
  grid.newpage()
  
  g1 <- graph (subset (data.allFaces, Node %in% nodes.good),     "Packets")
  g2 <- graph (subset (data.allFaces, Node %in% nodes.evil),     "Packets")
  g3 <- graph (subset (data.allFaces, Node %in% nodes.producer), "Packets")
    
  pushViewport(vpList(
    viewport(x = 0.5, y = .66, width = 1, height = .33,
      just = c("center", "bottom"), name = "p1"),
    viewport(x = 0.5, y = .33, width = 1, height = .33,
      just = c("center", "bottom"), name = "p2"),
    viewport(x = 0.5, y = .00, width = 1, height = .33,
      just = c("center", "bottom"), name = "p3")
    ))
  
  ## Add the plots from ggplot2
  upViewport()
  downViewport("p1")
  print(g1, newpage = FALSE)
  
  upViewport()
  downViewport("p2")
  print(g2, newpage = FALSE)
  
  upViewport()
  downViewport("p3")
  print(g3, newpage = FALSE)  
}

x = dev.off ()
cat ("Done\n")
