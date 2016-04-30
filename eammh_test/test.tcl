#==============================#
# Simulation parameters setup  #
#============================= #
set val(chan) Channel/WirelessChannel 
# channel type
set val(prop) Propagation/TwoRayGround 
# radio-propagation model
set val(netif) Phy/WirelessPhy
# network interface type
set val(mac) Mac
# MAC type
set val(ifq) Queue/DropTail/PriQueue 
# interface queue type
set val(ll) LL
# link layer type
set val(ant) Antenna/OmniAntenna
# antenna model
set val(ifqlen) 50
# max packet in ifq
set val(range) 50
set val(nn) 100
# number of nodes
set val(rp) EAMMH
# routing protocol
set val(x) 200
# X dimension of topography
set val(y) 200
# Y dimension of topography
set val(stop) 40.0
# time of simulation end
set val(energyModel) "EnergyModel"
# energy Model
set val(txPower) 0.02496
# receiving power consumed in W i.e., 24.96 mW  -----------------------------------specific to Mica2
set val(rxPower) 0.01872
# transmitting power consumed in W i.e., 18.72 mW
set val(initialEnergy) 30.0
# initial energy of the node in 30.0 J from a 3 V source with capacity 25 mAh

#====================#
#   Initialization   #
#====================#

#Create a ns simulator
set ns [new Simulator]

$ns use-newtrace

#Open the NS trace file
set tracefile [open test.tr w]
$ns trace-all $tracefile

#Open the NAM trace file
set namfile [open test.nam w]
#$ns namtrace-all $namfile
$ns namtrace-all-wireless $namfile $val(x) $val(y)

#Create wireless channel
set chan [new $val(chan)]

#Setup topography object
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)
set god [create-god [expr $val(nn) + 1]]

#Transmission range
$val(netif) set RXThresh_ $val(range)

#Define a 'finish' procedure
proc finish {} {
        global ns namfile tracefile
        $ns flush-trace
        close $namfile
	close $tracefile
        exec nam test.nam &
        exit 0
}

#===============================#
#  Mobile node parameter setup  #
#===============================#

$ns node-config -addressingType flat \
	-adhocRouting $val(rp) \
	-llType $val(ll) \
	-macType $val(mac) \
	-ifqType $val(ifq) \
	-ifqLen $val(ifqlen) \
	-antType $val(ant) \
	-propType $val(prop) \
	-phyType $val(netif) \
	-channel $chan \
	-topoInstance $topo \
	-agentTrace ON \
	-routerTrace ON \
	-macTrace OFF \
	-movementTrace OFF \
	-energyModel $val(energyModel) \
	-rxPower $val(rxPower) \
	-txPower $val(txPower) \
	-initialEnergy $val(initialEnergy)



Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1

#======================#
#   Nodes Definition   #
#======================#


# Random positions for nodes
proc myRand {} {
set max 200
set random_num [expr int([expr rand() * $max])]
return $random_num
}


### General Grid-Layout
### Create nodes

for {set i 0} {$i<$val(nn)} {incr i} {
  set node_($i) [$ns node]
  set xpos [myRand]
  set ypos [myRand]
	
  # disable random motion
  $node_($i) random-motion 0 
	
  $node_($i) set X_ $xpos  
  $node_($i) set Y_ $ypos
  $node_($i) set Z_ 0.0
	
  $ns initial_node_pos $node_($i) 10

  # attach nodes to god
  $god new_node $node_($i)
}


### Create Base Station

set base_stn [$ns node]
$base_stn set X_ 300.0
$base_stn set Y_ 100.0
$base_stn set z_ 0.0

$base_stn color blue

$ns at 0.0 "$base_stn color blue"
$ns at 0.0 "$base_stn label Base_Station"
$ns initial_node_pos $base_stn 10
# attach nodes to god
$god new_node $base_stn


### Attach UDP Agents and CBR traffic Model
### to each node

for {set i 0} {$i<$val(nn)} {incr i} {
set udp_($i) [new Agent/UDP]
$ns attach-agent $node_($i) $udp_($i)
set cbr_($i) [new Application/Traffic/CBR]
$cbr_($i) set packetSize_ 512
$cbr_($i) set interval_ 0.1
$cbr_($i) set random_ 1
$cbr_($i) set maxpkts_ 100
$cbr_($i) attach-agent $udp_($i)
}

### Start and Stop CBR traffic flow from each node

set start_time 0.4
set stop_time 0.9
set count 0

while { $count < [expr $val(stop) - 1]} {
for {set i 0} {$i<$val(nn)} {incr i} {
$ns at $start_time "$cbr_($i) start"
$ns at $stop_time "$cbr_($i) stop"
}
set start_time [expr $start_time + 1.0]
set stop_time [expr $stop_time + 1.0]
incr count
}

# call finish proc at stop of simulation
$ns at $val(stop) "finish"

#Run the simulation
$ns run
