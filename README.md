# 2822400Hz_PCM
A program to convert DSD to PCM without decimation

Ever wonder what goes on in the ultrasonic range of DSD audio? Let's find out. This simple program extracts the samples from a DSDIFF file and puts them in a PCM file unaltered so you can analyze it in your favourite audio editor. 

## Compile
    gcc -o dff2rawpcm dff2rawpcm.c

## Usage
    ./dff2rawpcm input.dff output.pcm

## Notes
- DST (compressed DSD) is not supported. Uncompressed DSD only.
- The output is 8 bit PCM so the output file size will be 8 times as big as the input file size.
