# OS Project – Graph Traffic Simulation

## Milestone 1 – Dijkstra
### Compile and Run
make milestone1
./milestone1/dijkstra <file>

### Description
Reads a directed weighted graph from a file and runs Dijkstra's
algorithm to find the shortest path between two nodes. Handles
disconnected graphs, negative weights, and same source/destination.

## Milestone 2 – Graph GUI
### Compile and Run
make milestone2
./milestone2/sim <file>

### Description
Displays the graph visually using raylib. Nodes shown as circles
with IDs, edges as directed arrows with weights. The shortest
path is highlighted in orange.

## Milestone 3 – Animation
### Compile and Run
make milestone3
./milestone3/sim <file>

### Description
Adds an animated entity (red circle) that travels along the
shortest path. Features a Play/Stop button. The entity moves
along each edge in W steps of 300ms each, and waits 1 second
at each intermediate node. Shows arrival message at destination.

## Clean
make clean

## Input File Format
N M
src1 dst1 weight1
...
source destination
