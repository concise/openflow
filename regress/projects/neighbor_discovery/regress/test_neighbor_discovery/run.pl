#!/usr/bin/perl -w
# test_neighbor_discovery
#
# Verify we get a discovery packet from the switch
# Then send the switch a discovery packet and verify we
# 	get the appropriate openflow discovery msg

use strict;
require OF::Includes;

my $LLDP_format = "a6 a6 n n a8 n";
my $LLDP_len = 26;

sub make_lldp
{
	my ($dpid, $out_port,$interval) = @_;
	my $pkt = pack $LLDP_format, (
		"01:80:C2:00:00:0E",	# DA
		"08:00:56:00:00:00",	# SA
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
	my @list = unpack $LLDP_format;
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

sub my_test {

	my ($sock, $options_ref) = @_;

	# this could be on any port.. just making sure it happens here
	my ($lldp, $lldp_packet, $out_lldp, $key);

	my $port_base = $$options_ref{'port_base'};
	my $in_port   = $port_base;
	my $out_port  = $in_port + 1;

	print "Sleeping for a bit to wait for lldp to show up from switch\n";
	sleep(10000);
	print "Waiting for lldp from switch\n";
	$lldp_packet = nftest_get_unexpected('eth1');
	die "Did not get an lldp packet from switch:$!" if(not defined($lldp_packet));
	die "Got an empty lldp packet from switch:$!" if($lldp_packet eq '');
	$lldp = &parse_lldp($lldp_packet);
	foreach $key (sort keys %$lldp)
	{
		print "$key :: " . $lldp->{$key} . "\n";
	}
	
	# build a neighbor discovery packet
	$out_lldp = &make_lldp("01:02:03:04:05:06:07:08",$out_port,10);
	
	die("Forced Death");

	# hello sequence automatically done by test harness!
}

run_black_box_test( \&my_test, \@ARGV );
