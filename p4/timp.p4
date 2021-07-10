/*
* This file is part of the TIMP distribution (https://github.com/nmaksic/timp).
* Copyright (c) 2020 Natasa Maksic.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

// P4_16 
#include <core.p4>
#include <v1model.p4>

const  bit<8>         PSA_PORT_RECIRCULATE = 254;

//#define HASH_EXP 24
//#define SINGLE_TABLE_EXP 23

// for testing
#define HASH_EXP 8
#define HASH_CONST_ONE 8w1
#define SINGLE_TABLE_EXP 7

#define ROUTING_PRICE_BITS 8

#define DST_COUNT 2
#define SW_COUNT 4
#define PORT_COUNT 4  // used for multicast group

const bit<16> TYPE_IPV4 = 0x800;
const bit<16> TYPE_ROUTING = 0x0111;
const bit<16> TYPE_LOCAL = 0x0112;
const bit<8> TYPE_TCP = 0x06;


const bit<32> HASH_SIZE = 32w1 << HASH_EXP;   
const bit<32> SINGLE_TABLE_SIZE = (32w1 << SINGLE_TABLE_EXP);
const bit<HASH_EXP> HASH_SEL_MASK = (HASH_CONST_ONE << (HASH_EXP-1));
const bit<HASH_EXP> HASH_ADR_MASK = ~(HASH_CONST_ONE << (HASH_EXP-1));

const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_RECIRC = 4;

#define IS_RECIRCULATED(std_meta) (std_meta.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_RECIRC)

/*************************************************************************
*********************** H E A D E R S  ***********************************
*************************************************************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

typedef bit<8> swId_t; 			  // up to 256 TOR switches
typedef bit<ROUTING_PRICE_BITS> routingPrice_t;  // max number of flows is equal to the total capacity of two used hash tables
typedef bit<32> registerIndex_t;

typedef bit<8> PortId_t;

typedef bit<48> time_t;
const time_t MAXTIME = 2000; // hash expiration in microseconds, value for testing

header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

header tcp_t {
  bit<16> srcPort;
  bit<16> dstPort;
  bit<32> seq;
  bit<32> ack;
  bit<4> dataofs;
  bit<3> reserved;
  bit<9> flags;
  bit<32> window;
  bit<16> chksum;
  bit<16> urgptr;
}

header update_t { 
    swId_t dst;
    routingPrice_t price;
} 


header update_local_t { 
	 PortId_t port1; // add flow to this port
	 PortId_t port2; // remove flow from this port
	 PortId_t port3; // remove flow from this port
} 

struct metadata {

    PortId_t outport;
    PortId_t altPort1; // first alternative port towards to packet destination
    PortId_t altPort2; // second alternative port towards to packet destination
    PortId_t altPort3; // thirt alternative port towards to packet destination
	 swId_t dstSw;			

    PortId_t localPort;
    bit<8> multicastGroupOffset;

}

struct headers {
    ethernet_t   ethernet;
    ipv4_t       ipv4;
    update_t     update;
	 update_local_t update_local;
	 tcp_t tcp;
}

struct empty_t {
}

error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}

/*************************************************************************
*********************** P A R S E R  ***********************************
*************************************************************************/

parser MyParser(packet_in packet,
                out headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {

    state start {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
				TYPE_ROUTING: parse_routing;
				TYPE_LOCAL: parse_local;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
		  transition select(hdr.ipv4.protocol) {
          TYPE_TCP: parse_tcp;
          default: accept;
		  }
    }

    state parse_routing {
        packet.extract(hdr.update);
        transition accept;
    }

    state parse_local {
        packet.extract(hdr.update_local);
        transition accept;
    }

	 state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
	 }

}

/*************************************************************************
************   C H E C K S U M    V E R I F I C A T I O N   *************
*************************************************************************/

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {   
    apply {  }
}


/*************************************************************************
**************  I N G R E S S   P R O C E S S I N G   *******************
*************************************************************************/

control MyIngress(inout headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata) {

		
	 // hash table 1
	 register<PortId_t>(SINGLE_TABLE_SIZE) outports1;
	 register<time_t>(SINGLE_TABLE_SIZE) ftimes1;

	 // hash table 2
	 register<PortId_t>(SINGLE_TABLE_SIZE) outports2;
	 register<time_t>(SINGLE_TABLE_SIZE) ftimes2;

	 // current pos of scanning in hash table 1
	 register<bit<SINGLE_TABLE_EXP>>(1) pos_hash1;

	 // current pos of scanning in hash table 2
	 register<bit<SINGLE_TABLE_EXP>>(1) pos_hash2;
		
	 // number of flows for each port
	 // different registers in order to access them in different stages
	 register<routingPrice_t>(1) flows_local_port1;
	 register<routingPrice_t>(1) flows_local_port2;
	 register<routingPrice_t>(1) flows_local_port3;
	 register<routingPrice_t>(1) flows_local_port4;

	 // maximal link utilization on the paths to destinations 
	 // received from neighbors
	 register<routingPrice_t>(DST_COUNT) flows_rcv_port1;   
	 register<routingPrice_t>(DST_COUNT) flows_rcv_port2;   
    register<routingPrice_t>(DST_COUNT) flows_rcv_port3;	 
	 register<routingPrice_t>(DST_COUNT) flows_rcv_port4;

	 // last maximal utilization on the best path to destination
	 // sent to neighbors
	 register<routingPrice_t>(DST_COUNT) advertizedmin_reg;

	 swId_t g_Dst = 0;
    swId_t g_thisSw = 0;

    action drop() {
        mark_to_drop(standard_metadata);
    }
     

	 action setoutput(PortId_t port) {
		standard_metadata.egress_spec = (egressSpec_t)port;
		meta.outport = port;
	 }
	

	 // action for ipv4_altports table
    action lfmp_altports(PortId_t altPort1, PortId_t altPort2, PortId_t altPort3, bit<8> multicastGroupOffset) {
        meta.altPort1 = altPort1;
	meta.altPort2 = altPort2;
	meta.altPort3 = altPort3;
	meta.multicastGroupOffset = multicastGroupOffset;
    }

	 // action for ipv4_lpm table
	 action lfmp_lpm(swId_t dstSw, swId_t thisSw, PortId_t localPort) {
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;  

		  g_Dst= dstSw;
		  g_thisSw = thisSw;
		  meta.dstSw = dstSw;
		  meta.localPort = localPort;
		  
    }
    
    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            lfmp_lpm;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

	 table ipv4_altports {
        key = {
            meta.dstSw: exact;
        }
        actions = {
            lfmp_altports;
            NoAction;
        }
        size = 1024;
        default_action = NoAction();
    }

    apply {

    if (!(hdr.ipv4.isValid() || hdr.update.isValid() || hdr.update_local.isValid())) {
		drop();		
	 }

    if (hdr.ipv4.isValid() || hdr.update.isValid() || hdr.update_local.isValid()) {

			if (hdr.ipv4.isValid()) {
				ipv4_lpm.apply();
			}	 

			if (hdr.update.isValid()) {
				g_Dst= hdr.update.dst+1;
				meta.dstSw = g_Dst; 
			}	 

			if (hdr.ipv4.isValid() || hdr.update.isValid()) {
				ipv4_altports.apply();
			}

			if (hdr.ipv4.isValid()) {
				if (g_Dst == g_thisSw) {
					setoutput(meta.localPort);
			   }
			}
	
	if (g_Dst != g_thisSw || hdr.update.isValid() || hdr.update_local.isValid()) {
	
		// get local prices and handle local updates
		routingPrice_t priceLocal1;
		routingPrice_t priceLocal2;
		routingPrice_t priceLocal3;
		routingPrice_t priceLocal4;
		routingPrice_t tmp;

		flows_local_port1.read(tmp, 0); 
		if (hdr.update_local.isValid() && hdr.update_local.port1 == (PortId_t)1) {
			tmp = tmp + 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port2 == (PortId_t)1) {
			tmp = tmp - 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port3 == (PortId_t)1) {
			tmp = tmp - 1;
		}
		flows_local_port1.write(0, tmp);
		priceLocal1 = tmp;

		flows_local_port2.read(tmp, 0); 
		if (hdr.update_local.isValid() && hdr.update_local.port1 == (PortId_t)2) {
			tmp = tmp + 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port2 == (PortId_t)2) {
			tmp = tmp - 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port3 == (PortId_t)2) {
			tmp = tmp - 1;
		}
		flows_local_port2.write(0, tmp);
		priceLocal2 = tmp;

		flows_local_port3.read(tmp, 0); 
		if (hdr.update_local.isValid() && hdr.update_local.port1 == (PortId_t)3) {
			tmp = tmp + 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port2 == (PortId_t)3) {
			tmp = tmp - 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port3 == (PortId_t)3) {
			tmp = tmp - 1;
		}
		flows_local_port3.write(0, tmp);
		priceLocal3 = tmp;

		flows_local_port4.read(tmp, 0); 
		if (hdr.update_local.isValid() && hdr.update_local.port1 == (PortId_t)4) {
			tmp = tmp + 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port2 == (PortId_t)4) {
			tmp = tmp - 1;
		}
		if (hdr.update_local.isValid() && hdr.update_local.port3 == (PortId_t)4) {
			tmp = tmp - 1;
		}
		flows_local_port4.write(0, tmp);
		priceLocal4 = tmp;

		// update processing is finished here and 
		// local update packet is marked to be dropped at the end of ingress
		if (hdr.update_local.isValid()) {
			drop();		
		}

		// this is skipped for local update packet
		// data packet or route update packet packets are processed
		if (hdr.ipv4.isValid() || hdr.update.isValid()) { 

			routingPrice_t pricePort1;
			routingPrice_t pricePort2;
			routingPrice_t pricePort3;
			routingPrice_t pricePort4;

			routingPrice_t priceRcv1;
			routingPrice_t priceRcv2;
			routingPrice_t priceRcv3;
			routingPrice_t priceRcv4;


			// rw price received on alternate port 1
			if (hdr.update.isValid() && standard_metadata.ingress_port == (egressSpec_t)1) {
				flows_rcv_port1.write((registerIndex_t)hdr.update.dst, hdr.update.price);   // (bit<32>)(DST_COUNT * (standard_metadata.ingress_port-1) + 
				tmp = (routingPrice_t)hdr.update.price;
			} else { // read from reg
				flows_rcv_port1.read(tmp, (registerIndex_t)hdr.update.dst); 
			}
			priceRcv1 = tmp;

			// rw price received on alternate port 2
			if (hdr.update.isValid() && standard_metadata.ingress_port == (egressSpec_t)2) {
				flows_rcv_port2.write((registerIndex_t)hdr.update.dst, hdr.update.price);   // (bit<32>)(DST_COUNT * (standard_metadata.ingress_port-1) + 
				tmp = (routingPrice_t)hdr.update.price;
			} else { // read from reg
				flows_rcv_port2.read(tmp, (registerIndex_t)hdr.update.dst); 
			}
			priceRcv2 = tmp;

			// rw price received on alternate port 3
			if (hdr.update.isValid() && standard_metadata.ingress_port == (egressSpec_t)3) {
				flows_rcv_port3.write((registerIndex_t)hdr.update.dst, hdr.update.price);   // (bit<32>)(DST_COUNT * (standard_metadata.ingress_port-1) + 
				tmp = (routingPrice_t)hdr.update.price;
			} else { // read from reg
				flows_rcv_port3.read(tmp, (registerIndex_t)hdr.update.dst); 
			}
			priceRcv3 = tmp;

			// rw price received on alternate port 4
			if (hdr.update.isValid() && standard_metadata.ingress_port == (egressSpec_t)4) {
				flows_rcv_port4.write((registerIndex_t)hdr.update.dst, hdr.update.price);   
				tmp = (routingPrice_t)hdr.update.price;
			} else { // read from reg
				flows_rcv_port4.read(tmp, (registerIndex_t)hdr.update.dst); 
			}
			priceRcv4 = tmp;
   

			// select best port and generate local and route update

			pricePort1 = priceLocal1 > priceRcv1 ? priceLocal1 : priceRcv1;
			pricePort2 = priceLocal2 > priceRcv2 ? priceLocal2 : priceRcv2;
			pricePort3 = priceLocal3 > priceRcv3 ? priceLocal3 : priceRcv3;
			pricePort4 = priceLocal4 > priceRcv4 ? priceLocal4 : priceRcv4;

			bool initialized = false;

			routingPrice_t minPrice = 0;
			PortId_t minPort = (PortId_t)0;


			if ((PortId_t)1 == meta.altPort1 || (PortId_t)1 == meta.altPort2 || (PortId_t)1 == meta.altPort3) {
				if (!initialized || pricePort1 < minPrice) {
					minPrice = pricePort1;
					minPort = (PortId_t)1;
					initialized = true;
				}
			}

			if ((PortId_t)2 == meta.altPort1 || (PortId_t)2 == meta.altPort2 || (PortId_t)2 == meta.altPort3) {
				if (!initialized || pricePort2 < minPrice) {
					minPrice = pricePort2;
					minPort = (PortId_t)2;
					initialized = true;
				}
			}

			if ((PortId_t)3 == meta.altPort1 || (PortId_t)3 == meta.altPort2 || (PortId_t)3 == meta.altPort3) {
				if (!initialized || pricePort3 < minPrice) {
					minPrice = pricePort3;
					minPort = (PortId_t)3;
					initialized = true;
				}
			}

			if ((PortId_t)4 == meta.altPort1 || (PortId_t)4 == meta.altPort2 || (PortId_t)4 == meta.altPort3) {
				if (!initialized || pricePort4 < minPrice) {
					minPrice = pricePort4;
					minPort = (PortId_t)4;
					initialized = true;
				}
			}

			time_t ct = standard_metadata.ingress_global_timestamp;

			// if route packet than check both tables
			bit<HASH_EXP> flowlet_hash;
			bool hashtable1 = false;
			bool hashtable2 = false;
			if (hdr.ipv4.isValid()) {
				
				hash(flowlet_hash, HashAlgorithm.crc16, 16w0, {
					hdr.ipv4.srcAddr,
					hdr.ipv4.dstAddr,
					hdr.ipv4.protocol,
					hdr.tcp.srcPort,		
					hdr.tcp.dstPort
				}, HASH_SIZE); 

				flowlet_hash = flowlet_hash & HASH_ADR_MASK;
				hashtable1 = ((flowlet_hash & HASH_SEL_MASK) > 0); // use highest bit to select one of two hash tables
				hashtable2 = ((flowlet_hash & HASH_SEL_MASK) == 0); // use highest bit to select one of two hash tables
				
			} 
		
		
			bool hashHit = false;
			PortId_t hashPort = 0;


			// simply chack hash in one table and check for expired flow based on round robin counter in another
			PortId_t localUpdatePort1 = 0; 
			PortId_t localUpdatePort2 = 0;
			PortId_t localUpdatePort3 = 0;

			bit<32> adrZero = 0; // use to access register at address 0
	
			bit<32> adr1; // address in first hash table
			if (hashtable1) {
				adr1 = (bit<32>)flowlet_hash;
			} else {
				// address for round robin checking 
				bit<SINGLE_TABLE_EXP> pos1 = 0;
				pos_hash1.read(pos1, adrZero);
				if ((bit<32>)pos1<SINGLE_TABLE_SIZE-1) {
					pos_hash1.write(0, pos1+1);
					adr1 = (bit<32>)(pos1+1);
				} else {
					pos_hash1.write(0, 0);
					adr1 = 0;
				}
			}

			time_t ft1;
			ftimes1.read(ft1, adr1);
			time_t exptime1 = ct - ft1;
			PortId_t outport1;
			outports1.read(outport1, adr1);
			if (hashtable1) {
				if (outport1 > 0) { 
					if (exptime1 < MAXTIME) {
						// hash hit
						// partial collision detection
						if (outport1 == meta.altPort1 || outport1 == meta.altPort2 || outport1 == meta.altPort3) {
							hashHit = true;
							ftimes1.write(adr1, ct);
							hashPort = outport1;
						}
					} else {
						// expired flow, replace with new, no change for this dst
						outports1.write(adr1, minPort);			
						ftimes1.write(adr1, ct);	
						if (outport1 != minPort) {
							localUpdatePort1 = minPort; 
							localUpdatePort2 = outport1;
						}	
					}
	 			} else {
					// new flow 
					outports1.write(adr1, minPort);			
					ftimes1.write(adr1, ct);				
					localUpdatePort1 = minPort; 
				}
			} else {
				// round robin checking
				if (exptime1 > MAXTIME) {
					if (outport1 > 0) { 
							outports1.write(adr1, 0);
							localUpdatePort3 = outport1;
					}
				}
			
			}
		
			bit<32> adr2;
			if (hashtable2) {
				adr2 = (bit<32>)flowlet_hash;
			} else {
				// address for round robin checking 
				bit<SINGLE_TABLE_EXP> pos2 = 0;
				pos_hash2.read(pos2, adrZero);
				if ((bit<32>)pos2 < SINGLE_TABLE_SIZE-1) {
					pos_hash2.write(0, pos2+1);
					adr2 = (bit<32>)(pos2+1);
				} else {
					pos_hash2.write(0, 0);
					adr2 = 0;
				}
			}

			// if hash entry is in hash table 2
			time_t ft2;
			ftimes2.read(ft2, adr2);
			time_t exptime2 = ct - ft2;
			PortId_t outport2;
			outports2.read(outport2, adr2);
			if (hashtable2) {
				if (outport2 > 0) { 
					if (exptime2 < MAXTIME) {
						// hash hit
						// partial collision detection
						if (outport2 == meta.altPort1 || outport2 == meta.altPort2 || outport2 == meta.altPort3) {
							hashHit = true;
							ftimes2.write(adr2, ct);
							hashPort = outport2;
						}
					} else {
						// expired flow, replace with new, no change for this dst
						outports2.write(adr2, minPort);			
						ftimes2.write(adr2, ct);	
						if (outport2 != minPort) {
							localUpdatePort1 = minPort; 
							localUpdatePort2 = outport2;
						}	
					}
	 			} else {
					// new flow 
					outports2.write(adr2, minPort);			
					ftimes2.write(adr2, ct);				
					localUpdatePort1 = minPort; 
				}
			} else {
				// round robin checking
				if (exptime2 > MAXTIME) {
					if (outport2 > 0) { 
						outports2.write(adr2, 0);
						localUpdatePort3 = outport2;
					}
				}
			
			}
 
			PortId_t sendPort = minPort;
			if (hashHit) {
				sendPort = hashPort;
			}
			setoutput(sendPort);

			// apply flow changes to calculated min price, it can be maximally incremented by one
			if (hdr.ipv4.isValid()) {
				if (localUpdatePort1 == minPort)
					minPrice = minPrice + 1;
				if (localUpdatePort2 == minPort)
					minPrice = minPrice - 1;
				if (localUpdatePort3 == minPort)
					minPrice = minPrice - 1;			
			}

			routingPrice_t oldPrice;


			// before sending update, check if this price has already been sent
			advertizedmin_reg.read(oldPrice, (bit<32>)(g_Dst-1));	
			if (oldPrice != minPrice) 
				advertizedmin_reg.write((bit<32>)(g_Dst-1), minPrice);

			if (oldPrice != minPrice) { 

				hdr.update.setValid(); 
				hdr.update.dst = (swId_t)(g_Dst-1); 
				hdr.update.price = (routingPrice_t)minPrice;

			} else {
				hdr.update.setInvalid(); 
				if (!hdr.ipv4.isValid())
					drop();
			}

			if (localUpdatePort1 != 0 || localUpdatePort2 != 0 || localUpdatePort3 != 0) {
				hdr.update_local.setValid(); 
				hdr.update_local.port1 = localUpdatePort1;
				hdr.update_local.port2 = localUpdatePort2;
				hdr.update_local.port3 = localUpdatePort3;		
			}


		if (hdr.update.isValid() || hdr.update_local.isValid()) { // we use multicast in order to generate updates
			standard_metadata.mcast_grp = (bit<16>)(meta.multicastGroupOffset + sendPort ); 
		}

	
		}

	} //if (hdr.ipv4.isValid() || hdr.update.isValid()) { 

	} //if (g_Dst != g_thisSw || hdr.update.isValid() || hdr.update_local.isValid()) {


	} // apply
} // control


/*************************************************************************
****************  E G R E S S   P R O C E S S I N G   *******************
*************************************************************************/


control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {



   apply { 		

		

		PortId_t dstPort = (PortId_t)standard_metadata.egress_port;


		if ( hdr.ipv4.isValid()) {
			if ((PortId_t)meta.outport == dstPort) {
				hdr.update.setInvalid();			
				hdr.update_local.setInvalid();
			}
		}

		if ( hdr.update.isValid()) {
			if (dstPort != PSA_PORT_RECIRCULATE) {
				hdr.ipv4.setInvalid();			
				hdr.tcp.setInvalid();
				hdr.update_local.setInvalid();		
				hdr.ethernet.etherType = TYPE_ROUTING;
			}
		}

		if ( hdr.update_local.isValid()) {
			if (dstPort == PSA_PORT_RECIRCULATE) {
				hdr.ipv4.setInvalid();
				hdr.tcp.setInvalid();
				hdr.update.setInvalid();
				hdr.update_local.setValid();
				hdr.ethernet.etherType = TYPE_LOCAL; 
				recirculate(standard_metadata);
			}
		} 

    } // apply
}

/*************************************************************************
*************   C H E C K S U M    C O M P U T A T I O N   **************
*************************************************************************/

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
     apply {
	update_checksum(
	    hdr.ipv4.isValid(),
            { hdr.ipv4.version,
	      hdr.ipv4.ihl,
              hdr.ipv4.diffserv,
              hdr.ipv4.totalLen,
              hdr.ipv4.identification,
              hdr.ipv4.flags,
              hdr.ipv4.fragOffset,
              hdr.ipv4.ttl,
              hdr.ipv4.protocol,
              hdr.ipv4.srcAddr,
              hdr.ipv4.dstAddr },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16);
    }
}


/*************************************************************************
***********************  D E P A R S E R  *******************************
*************************************************************************/

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
		  packet.emit(hdr.tcp);
        packet.emit(hdr.update);
		  packet.emit(hdr.update_local);
    }
}

/*************************************************************************
***********************  S W I T C H  *******************************
*************************************************************************/

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
