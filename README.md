# OS Project – Graph Traffic Simulation

## Milestone 1 – Dijkstra
### Compile and Run
```bash
make milestone1
./milestone1/dijkstra <file_name>
```

### Description
Reads a directed weighted graph from a file and runs Dijkstra's
algorithm to find the shortest path between two nodes. Handles
disconnected graphs, negative weights, and same source/destination.

## Milestone 2 – Graph GUI
### Compile and Run
```bash
make milestone2
./milestone2/sim <file_name>
```

### Description
Displays the graph visually using raylib. Nodes shown as circles
with IDs, edges as directed arrows with weights. The shortest
path is highlighted in orange.

## Milestone 3 – Animation
### Compile and Run
```bash
make milestone3
./milestone3/sim <file_name>
```

### Description
Adds an animated entity (red circle) that travels along the
shortest path. Features a Play/Stop button. The entity moves
along each edge in W steps of 300ms each, and waits 1 second
at each intermediate node. Shows arrival message at destination.

## Milestone 4 – Multiple Travelers with fork()
### Compile and Run
```bash
make milestone4
./milestone4/sim <file_name>
```
### Description
Parent process reads the graph and computes Dijkstra for each traveler.
Uses fork() to create one child per traveler. Each child prints [PID] started
then sleeps. All travelers animate simultaneously in different colors in the GUI.
Parent sends SIGTERM to children when animation ends and waits for them.

## Milestone 5 – IPC with Pipes
### Compile and Run
```bash
make milestone5
./milestone5/sim <file_name>
```
### Description
Each child computes its own Dijkstra path independently. Children send position
updates to the parent via pipes. Parent reads the pipes, prints the log to
terminal, and updates the GUI. IPC method: pipes. Chosen because pipes are
simple, reliable, and well-suited for one-directional child-to-parent
communication without shared memory complexity.

## Milestone 6 – Node Synchronization
### Compile and Run
```bash
make milestone6
./milestone6/sim <file_name>
```
### Description
Adds synchronization so at most one traveler can be in a node at a time.
Uses POSIX semaphores in shared memory (shm_open + mmap), one per node.
Before entering a node, each child sends MSG_WAITING then calls sem_wait().
After the 1-second stay, it calls sem_post() to release the node.
GUI shows waiting travelers in yellow and done travelers in gray.
IPC method: pipes. Synchronization: POSIX semaphores in shared memory.

## Milestone 7 – Scheduling Algorithms
### Compile and Run
```bash
make milestone7
./milestone7/sim -schd fcfs <file_name>
./milestone7/sim -schd sjf <file_name>
```
### Description
Replaces random node entry order with scheduling algorithms. The parent
manages a waiting queue per node and grants entry based on the chosen
algorithm. Two algorithms are implemented:

- FCFS (First Come First Served): travelers enter the node in the order
  they requested it. The first to arrive is the first to enter.

- SJF (Shortest Job First): travelers enter based on their remaining
  path length. The traveler with the fewest steps left enters first.

### Comparison
With FCFS, a traveler with a long remaining path can block others simply
by arriving first. With SJF, travelers closer to their destination are
prioritized, reducing overall waiting time for shorter jobs. In our tests,
SJF clearly changed the entry order at the bottleneck node compared to FCFS,
demonstrating that shorter remaining paths get priority regardless of arrival time.

## Clean
```bash
make clean
```

## Input File Format
### Milestone 1

```

N M

src1 dst1 weight1

...

src dst

```
### Milestones 2-7

```

# graph definition

N M

src1 dst1 weight1

...

# travelers

K

src1 dst1

src2 dst2

```
