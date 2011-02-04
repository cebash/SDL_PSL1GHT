#! /usr/bin/perl

use strict;

my ($file, @resources) = @ARGV;

#Read the file
open FILE, $file or die "unknown file '$file'";
my @lines = <FILE>;
close FILE;

#if file needs a resource
my $needs_patch = 0;
foreach my $resource ( @resources)
{
	my @resln = grep( /$resource/, @lines);
	$needs_patch = 1 if ($#resln > -1);
}

#patch if was not already patched
my @dirdef = grep( /define DIR/, @lines);
if( $needs_patch && $#dirdef == -1)
{
	print "Patching $file (to add a directory in the resource path)\n";

#Patch & write the file
	open FILE, ">$file";
	print FILE "#ifndef DIR\n";
	print FILE "#define DIR\n";
	print FILE "#endif\n";

	foreach my $line (@lines)
	{
		my $patched_line = $line;
		foreach my $resource ( @resources)
		{
			$patched_line=~ s/"$resource"/DIR "$resource"/g;
		}
		print FILE $patched_line ;
	}
	close FILE;
}

