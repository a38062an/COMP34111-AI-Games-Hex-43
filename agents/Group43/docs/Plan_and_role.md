Team 43 Hex Agent: Project Overview & Role Definition

Project Goal: Develop a high-performance autonomous agent for the board game Hex (11x11) capable of competing in a resource-constrained environment.
Core Architecture: Hybrid Monte Carlo Tree Search (MCTS) with Deep Learning Policy Guidance and Tactical Pruning.
Target Metrics: 75% Win Rate / 25% Speed Efficiency (Strict 5-minute timebank).

1. The Game: Hex (11x11)

Hex is a connection strategy game played on a hexagonal grid.

Objective: Connect your two opposing sides with a continuous chain of stones.

The Board: An 11x11 rhombus of hexagons.

The Challenge:

High Branching Factor: ~121 possible opening moves.

No Draws: Every game must end in a win or loss.

Tactical Depth: Connections can be subtle (virtual connections), requiring deep lookahead.

2. Our "Advanced Approach" (Hybrid MCTS)

Instead of a standard "vanilla" MCTS, we are implementing a Hybrid Architecture inspired by state-of-the-art engines like MoHex 2.0. This approach divides decision-making into three specialized subsystems:

The "Math" Layer (Tactical Solver):

Responsible: Joshua Mamelok

Function: Uses deterministic algorithms (H-Search, Virtual Connections) to instantly identify forced wins or "must-play" defensive moves. It solves sub-problems mathematically, saving the search engine from wasting time on obvious tactics.

The "Brain" Layer (MCTS Engine):

Responsible: Anthony Nguyen

Function: The central decision-maker. It builds a search tree to explore future game states. It uses RAVE (Rapid Action Value Estimation) to speed up learning by sharing information across different branches of the tree.

The "Intuition" Layer (Neural Policy - YOUR ROLE):

Responsible: Alexios (You)

Function: A Deep Convolutional Neural Network (CNN) that looks at the board and predicts where a pro player would move.

Benefit: It effectively prunes the search tree by focusing the MCTS on the top 5-10 promising moves instead of randomly exploring all 100+ options.

The "Speed" Layer (Simulation Strategy):

Responsible: Alex Mote

Function: Runs fast simulations (playouts) from leaf nodes to determine a winner. Instead of random moves, it uses lightweight $3 \times 3$ pattern matching to play somewhat intelligently without the heavy cost of a neural network.

3. Your Specific Role: Neural Network Lead

Primary Objective: Build the "Intuition" of the agent.
You are not building the whole player; you are building the component that tells the search engine where to look first.

3.1 Key Responsibilities

Data Factory (HPC):

You are leveraging the University's CSF3 High-Performance Computing Cluster (A100 GPUs).

You will generate a massive synthetic dataset (~50,000 games) by running KataHex in self-play mode.

Why? Human data is too weak. Self-play data from a superhuman engine provides "soft targets" (probability distributions) that teach your network nuance.

Model Architecture (The "Goldilocks" CNN):

You must design a Convolutional Neural Network (CNN) that balances Accuracy vs. Latency.

Constraint: The final agent runs on a CPU in a Docker container.

Target: Inference must take < 2ms. A massive ResNet-50 is too slow; a tiny MobileNet might be too dumb. You will find the optimal middle ground.

The Output (Policy Priors):

Your network takes the $11 \times 11$ board state as input.

It outputs a Probability Vector (a list of 121 numbers summing to 1).

Example: [A1: 0.01, ..., F5: 0.85, ..., K11: 0.00]

This vector acts as the "Prior Probability" ($P$) in the MCTS selection formula ($UCT + P$).

Integration (ONNX Bridge):

Since the main engine is in C++, you cannot just "import torch".

You will export your trained model to the ONNX (Open Neural Network Exchange) format.

This allows the C++ engine to run your Python-trained brain at native C++ speeds with zero overhead.

4. The Critical Path (How it works in practice)

When it is our turn to move, the pipeline executes as follows:

Step 1: The Tactical Check (Joshua)

Does the solver see a forced win?

Yes: Play it immediately. (0ms used).

No: Pass board to Neural Network.

Step 2: The Intuition Pass (You)

The C++ engine feeds the board to your ONNX model.

Your model returns a heatmap of "good ideas."

Latency Target: 2ms.

Step 3: The Search (Anthony & Alex)

The MCTS builds a tree.

It looks at your heatmap. It sees you rated move F5 as 80%.

It immediately expands F5 and runs simulations (using Alex's patterns) to verify if F5 actually leads to a win.

It barely glances at move A1 because you rated it 1%.

Step 4: The Decision

After 5 seconds, the engine picks the most robust move found.

5. Why this Approach Wins Marks

Approach (50%): Using a Hybrid MCTS with distinct layers for Tactics (Math), Policy (NN), and Search (RAVE) is the "State-of-the-Art" architecture (MoHex 2.0 style).

Understanding (30%): Your use of the HPC for Training vs. ONNX for CPU Inference demonstrates deep understanding of resource constraints and deployment engineering.

Evidence: Your data generation strategy (KataHex Self-Play) proves you are making evidence-based decisions rather than guessing.