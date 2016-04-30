#!/usr/bin/python

# Open energy_trace.tr file for reading
fo_in = open("energy_trace.tr", "r")
# Open energy_graph.xg file for writing
fo_out = open("energy_graph.xg", "a")

fo_out.write("TitleText: Energy Level of Nodes at different Rounds\n")

count = 1
node_index = 0
node_energy = 0.0

for line in fo_in:
	value = line.split(" ")
	if(value[0] == "0"):
		fo_out.write("\n\"Round ")
		str_count = str(count)
		fo_out.write(str_count)
		fo_out.write("\n")
		count+=1
	fo_out.write(line)

# Close opend file
fo_in.close()
fo_out.close()
