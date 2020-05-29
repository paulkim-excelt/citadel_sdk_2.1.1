#include <stdio.h>

int main(int argc, char **argv) 
{
   char *infile, *outfile;
   FILE *inp, *outp;
   unsigned int data;
   int nb;

   if(argc >= 3) {
      argc--; argv++;
      infile = *argv;
      argc--; argv++;
	  outfile = *argv;
	  printf("In file: %s, Out file: %s\n", infile, outfile);
      inp = fopen(infile, "rb");
	  if(inp == NULL) {
		  printf("Error opening file %s\n", infile);
		  return(0);
	  }
      outp = fopen(outfile, "wb");
	  if(outp == NULL) {
		  printf("Error opening file %s\n", outfile);
		  return(0);
	  }
	  do {
	     nb = fread(&data, 1, 4, inp);
	     if(nb == 4) 
	        data = (data >> 24) | ((data >> 8) & 0xFF00) | ((data << 8) & 0xFF0000) | (data << 24);
	     if(nb > 0)
		    fwrite(&data, 1, nb, outp);
	  }while(nb == 4);

	  fclose(inp);
	  fclose(outp);
      return(0);
   }

   printf("Usage: big2little <inputfile> <outputfile>\n");
}
