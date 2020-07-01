In this document, we will briefly introduce our framework `GraphWalker` and the instructions for making it run.

## GraphWalker

`Graphwalker` is an I/O-efficient and resource-friendly graph analytic system for fast and scalable random walks.

### Introduction
Traditional graph systems mainly use the iteration-based model which iteratively loads graph blocks into memory for analysis so as to reduce random I/Os. However, this iteration-based model limits the efficiency and scalability of running random walk, which is a fundamental technique to analyze large graphs. In this paper, we propose GraphWalker, an I/O-efficient graph system for random walks by deploying a novel state-aware I/O model with asynchronous walk updating. GraphWalker is efficient to handle very large disk-resident graphs consisting of hundreds of billions of edges with only a single commodity machine, and it is also scalable to run tens of billions of random walks with thousands of steps long. Experiments on our prototype system show that GraphWalker can achieve more than an order of magnitude speedup when running a large amount of long random walks when compared with DrunkardMob, which is tailored for random walk based on the classical system GraphChi, as well as two state-of-the-art single-machine graph systems, Graphene and GraFSoft. Furthermore, comparing with the most recent distributed system KnightKing, which optimizes for random walks and runs on cluster machines, GraphWalker achieves comparable performance with only a single machine, thereby making it a more cost-effective alternative.

### Publication

Rui Wang, Yongkun Li, Hong Xie, Yinlong Xu, John C.S. Lui. GraphWalker: An I/O-Efficient and Resource-Friendly Graph Analytic System for Fast and Scalable Random Walks. ATC2020. [paperlink](https://www.usenix.org/conference/atc20/presentation/wang-rui)

### Try out `GraphWalker`

Next, we will first present instructions to install `GraphWalker` atop Ubuntu operating system. Second, we will present the instructions to run applications atop `GraphWalker`.

#### Compiling 

Using the following commands, one can easily compile the `GraphWalker`, the compiled applications are located at `bin/apps`

```bash
$ git clone https://github.com/rwang067/GraphWalker.git
$ cd GraphWalker
$ make
```

#### Dataset preparing

```bash
$ mkdir Dataset && cd Dataset
$ wget https://snap.stanford.edu/data/soc-LiveJournal1.txt.gz
$ gunzip soc-LiveJournal1.txt.gz
```

#### Performing apps atop `GraphWalker`

Here, we will take application `msppr` as an example to show how to run applications atop `GraphWalker`.

```bash
$ cd ../
$ LOG_LEVEL="debug" ./bin/apps/msppr file ./Dataset/soc-LiveJournal1.txt firstsource 0 numsources 1 walkspersource 2000 maxwalklength 10 prob 0.2
```