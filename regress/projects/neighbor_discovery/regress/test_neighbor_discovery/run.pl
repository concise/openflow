#!/usr/bin/perl -w
# test_neighbor_discovery
#
# 1) Send the switch a discovery packet and verify we
# 	get the appropriate openflow discovery msg
#
# 2) Wait to make sure we get a neighbor expire
#
# 3) Then Verify we get a discovery packet from the switch

use strict;
use OF::Includes;

my $LLDP_len = 26;
my $LLDP_format = "a6 a6 n n a8 n";
my $LLDP_destaddr = "\x01\x80\xC2\x00\x00\x0E";
my $LLDP_srcaddr = "\x08\x00\x56\x00\x00\x00";


sub make_lldp
{
	my ($dpid, $out_port,$interval) = @_;
	my $pkt = pack $LLDP_format, (
		$LLDP_destaddr,	# DA
		$LLDP_srcaddr,	# SA
		0x88cc,			# ethertype
		$out_port,		# outgoing port
		$dpid,			# datapath id
		$interval		# timeout period
	);
	return $pkt;
}

sub parse_lldp
{
	my ($packet) = @_;
	my @list = unpack $LLDP_format, $packet;
	my %lldp;

	die "Invalid lldp $packet :: not $LLDP_len bytes long" 
		unless(length($packet) == $LLDP_len);
	
	$lldp{"DA"} = $list[0];
	$lldp{"SA"} = $list[1];
	$lldp{"Ethertype"} = $list[2];
	$lldp{"out_port"} = $list[3];
	$lldp{"dpid"} = $list[4];
	$lldp{"interval"} = $list[5];

	return \%lldp;
}
sub dump_lldp
{
	my ($lldp) = @_;
	my $key;
	print "####### LLDP packet dump\n";
	foreach $key (sort keys %$lldp)
	{
		if($key eq 'header')
		{
			foreach $key (sort keys %{$lldp->{'header'}})
			{
				print "header: $key :: " . $lldp->{'header'}->{$key} . "\n";
			}
		}
		else
		{
			print "$key :: " . $lldp->{$key} . "\n";
		}
	}
}

sub my_test {

	my ($sock, $options_ref) = @_;

	# this could be on any port.. just making sure it happens here
	my ($lldp, $lldp_packet, $out_lldp, $key, $recvd_mesg, @unexpected_packets);

	my $port_base = $$options_ref{'port_base'};
	my $in_port   = $port_base;
	my $out_port  = $port_base;

	print "port_base = $port_base\n";
	print "in_port = $in_port\n";
	print "out_port = $out_port\n";

	############ test 1
	# 1) build a neighbor discovery packet, 
	# 2) send it in a port, 
	# 3) verify we get a new neighbor message
	$out_lldp = &make_lldp( "\x01\x02\x03\x04\x05\x06\x07\x08" , $out_port, 1);
	nftest_send( 'eth' . $out_port, $out_lldp);
	sysread( $sock, $recvd_mesg, 1512 ) || die "Failed to receive new neighbor message: $!";
	
	my $msg = $ofp->unpack( 'ofp_neighbor_msg', $recvd_mesg );
	dump_lldp($msg);
	die "Got command $msg->{'activity'} instead  of OFPNA_NEIGHBOR_DISCOVERED:$!" 
		unless ($msg->{'activity'} == $enums{'OFPNA_NEIGHBOR_DISCOVERED'});
	############## test 2
	# 1) sleep longer than the timeout
	# 2) verify we got the neighbor expired mesg
	sleep(2);
	sysread( $sock, $recvd_mesg, 1512 ) || die "Failed to receive new neighbor message: $!";
	
	$msg = $ofp->unpack( 'ofp_neighbor_msg', $recvd_mesg );
	die "Got command $msg->{'activity'} instead  of OFPNA_NEIGHBOR_EXPIRED:$!" 
		unless ($msg->{'activity'} == $enums{"OFPNA_NEIGHBOR_EXPIRED"});
	dump_lldp($msg);

	############## test 3
	# 1) make sure we got a neighbor lldp message from the switch at some point
	print "Waiting 12 sec for lldp from switch\n";
	sleep(12);
	@unexpected_packets = nftest_get_unexpected("eth" . $out_port);
	print "Potential lldp packets = " . join(',', @unexpected_packets) . "\n";
	die "Did not get an lldp packet from switch:$!" unless(@unexpected_packets > 0);
	$lldp_packet = $unexpected_packets[0];	# assumes the only stray packets in this test are lldp
	die "Got an empty lldp packet from switch:$!" if($lldp_packet eq '');
	$lldp = &parse_lldp($lldp_packet);

	die "Got lldp from switch with out_port $lldp->{'out_port'} instead  of 2:$!" 
		unless ($lldp->{'out_port'} == $out_port);
	print "LLDP has valid out_port\n";

	die "Got lldp from switch with dest ether $lldp->{'DA'} instead  of lldp_multicast addr:$!" 
		unless ($lldp->{'DA'} == $LLDP_destaddr);
	print "LLDP has valid ether daddr\n";

	die "Got lldp from switch with dest ether $lldp->{'SA'} instead  of lldp_multicast addr:$!" 
		unless ($lldp->{'SA'} == $LLDP_srcaddr);
	print "LLDP has valid ether saddr\n";

	print "Cleaning up LLDP on other interfaces\n";
	my $int;
	for $int ( ('eth1', 'eth2', 'eth3', 'eth4'))
	{
		# ignore packets on other all other interfaces
		@unexpected_packets = nftest_get_unexpected($int);
		for $msg (@unexpected_packets)	# stoopid testing harness
		{
			if( length($msg) == $LLDP_len)	# if it's an lldp packet
			{
				nftest_expect($int, $msg, 1);	# ignore it; 1 == dont pad packet
			}
		}
		print "Ignoring " . scalar(@unexpected_packets) . " lldp packets on interface $int\n";
	}
	#die("Forced Death"); for debugging
}

run_black_box_test( \&my_test, \@ARGV );
