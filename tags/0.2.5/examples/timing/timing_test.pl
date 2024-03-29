#!/usr/bin/perl

# The range of grid sizes to consider
@range=(10..40);

# The number of trials to consider. If this is set to one, the time
# for a single trial will be outputted. For higher values, the mean
# of all the trials will be outputted, along with the standard
# deviation.
$tries=1;

# The flags to pass for code optimization. The second line is appropriate for
# Mac OS X, forcing more function inlining, and enabling the Apple-only "-fast"
# flag for maximum code optimization.
$opt="-O3";
#$opt+="-fast --param large-function-growth=1000 --param max-inline-insns-single=2000";

foreach $r (@range) {

	# Compile the code with the current grid size
	system "g++ -I../../src -DNNN=$r -o timing_test $opt timing_test.cc";

	# Carry out the trials for this grid size 
	$st=$stt=0;
	foreach $t (1..$tries) {

		# Run the code, and output the timing information to the
		# "time_temp" file.
		system "./timing_test >time_temp";

		# Read the "time_temp" file to find the duration of the run
		open F,"time_temp" or die "Can't open timing file: $!";
		($t)=split ' ',<F>;
		$st+=$t;$stt+=$t*$t;
		close F;
	}

	# Compute the mean and variance and print to standard output
	$st/=$tries;
	$stt=$stt/$tries-$st*$st;$stt=$stt>0?sqrt($stt):0;
	print "$r $st $stt\n";
}

# Delete the temporary timing file
system "rm time_temp";
