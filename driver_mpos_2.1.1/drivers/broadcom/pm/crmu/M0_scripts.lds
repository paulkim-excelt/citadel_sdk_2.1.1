MEMORY
{
    M0_RAM (rwx) : ORIGIN = 0x30010000, LENGTH = 0x4000
}

SECTIONS
{
  .text :
  {
  	*.o (.start_vector)
	. = ALIGN(0x100);
    *.o (.text*)
  } > M0_RAM
}

