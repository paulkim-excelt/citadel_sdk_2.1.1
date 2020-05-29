my $in = $ARGV[0];
my $out = $ARGV[1];

my $str1, $len;
#      {
#       local $/ = undef;
       my $fsize = -s $in;
       open(my $inp, "<", "$in") or die "can't open in file";
       open(my $outp, ">", "$out") or die "can't open out file";
       my $nb;
       seek($inp,-9,2);
       $fsize -= 1; # to discard new line (0x0A)

       while(1) {
          $nb1 = read($inp,$str1,8);
          if($nb1 != 8) {
             last;
          }
          $str1 = "0x".$str1.",   ";
          print $outp $str1;

          $fsize -= 8;
          if($fsize <= 0) {
             last;
          }

          seek($inp,-16,1);
          $nb1 = read($inp,$str1,8);
          if($nb1 != 8) {
             last;
          }
          $str1 = "0x".$str1.",   ";
          print $outp $str1;

          $fsize -= 8;
          if($fsize <= 0) {
             last;
          }

          seek($inp,-16,1);
          $nb1 = read($inp,$str1,8);
          if($nb1 != 8) {
             last;
          }
          $str1 = "0x".$str1.",   ";
          print $outp $str1;

          $fsize -= 8;
          if($fsize <= 0) {
             last;
          }

          seek($inp,-16,1);
          $nb1 = read($inp,$str1,8);
          if($nb1 != 8) {
             last;
          }
          $str1 = "0x".$str1.",";
          print $outp $str1;
          print $outp "\n";

          $fsize -= 8;
          if($fsize <= 0) {
             last;
          }
          seek($inp,-16,1);
       }
#      }
close($inp);

close($outp );