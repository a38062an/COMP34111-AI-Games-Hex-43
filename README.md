# COMP34120 AI and Games Coursework - Group 43

This repository contains the coursework submission for Group 43.

## Agent Information
The source code for our agent is located in `agents/Group043`.

Please refer to `agents/Group043/README.md` for detailed documentation regarding the agent's architecture, build instructions, and strategy.

## Environment Setup
We will be running each agent within a specific Docker environment. Ensure your agent works within this container and adheres to the specified constraints.

### Building the Docker Image
To build the Docker image:

```bash
docker build --build-arg UID=$UID -t hex .
```

### Running the Container
To run the container:

```bash
docker run --cpus=8 --memory=8G -v "$(pwd)":/home/hex --name hex --rm -it hex /bin/bash
```
The current repository will be mapped to `/home/hex` within the container.

### Running a Game
To run a game of Hex:

```bash
python3 Hex.py
```
By default, two `agents/DefaultAgents/NaiveAgent.py` agents will play against each other. Use `python3 Hex.py --help` to see all available options.

## Testing
To run the test suite:

```bash
python3 -m unittest discover
```
