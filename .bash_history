python3 Hex.py
python3 Hex.py --help
python3 -m unittest discover
python3 Hex.py --help
python3 Hex.py --agent1 agents/DefaultAgents/ExternalAgent.py --agent2 agents/DefaultAgents/NaiveAgent.py
# Inside the container:
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
ls
ls
# Inside the container:
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
make -C agents/Group43/src clean && make -C agents/Group43/src
python3 Hex.py -p1 "agents.Group43.ExternalAgent Group43Agent" -p2 "agents.DefaultAgents.NaiveAgent NaiveAgent" -b 11 -v
