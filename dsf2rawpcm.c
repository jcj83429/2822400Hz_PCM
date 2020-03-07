#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SACD_BLOCK_SIZE_PER_CHANNEL         4096   // 4096 bytes per channel
#define BUFFER_SAMPLES SACD_BLOCK_SIZE_PER_CHANNEL*8
#define CHANNELS 2

int main(int argc, char *argv[]){
	FILE *infile, *outfile;
	uint8_t *inbuf, *outbuf;
	uint8_t tmp;
	int i, j, k, n;
	uint32_t channels, samplerate;
	int64_t samplecount;
	uint8_t chunkname[5] = {0};
	uint32_t tmp32;
	uint64_t tmp64;

	if(argc != 3){
		printf("Usage: %s input.dsf output.pcm\n", argv[0]);
		return 1;
	}
	infile = fopen(argv[1], "r");
	if(infile == NULL){
		printf("cannot open input\n");
		return 1;
	}
	outfile = fopen(argv[2], "w");
	if(outfile == NULL){
		printf("cannot open output\n");
		return 1;
	}
	
	fread(chunkname, 1, 4, infile);
	if(strcmp("DSD ", chunkname)){
		printf("not a DSF file\n");
		return 1;
	}
	
	fseek(infile, 28, SEEK_SET);
	
	fread(chunkname, 1, 4, infile);
	if(strcmp("fmt ", chunkname)){
		printf("fmt chunk not found\n");
		return 1;
	}
	
	fread(&tmp64, 1, 8, infile);
	if(tmp64 != 52){
		printf("fmt chunk size is not 52\n");
		return 1;
	}
	
	fread(&tmp32, 1, 4, infile);
	if(tmp32 != 1){
		printf("fmt chunk version is not 1\n");
		return 1;
	}
	
	fread(&tmp32, 1, 4, infile);
	if(tmp32 != 0){
		printf("format ID is not 0\n");
		return 1;
	}
	
	fseek(infile, 4, SEEK_CUR);
	fread(&channels, 1, 4, infile);
	printf("channels: %d\n", channels);
	
	fread(&samplerate, 1, 4, infile);
	printf("samplerate: %d\n", samplerate);
	
	fread(&tmp32, 1, 4, infile);
	if(tmp32 != 1){
		printf("bits per sample is not 1\n");
		return 1;
	}
	
	fread(&samplecount, 1, 8, infile);
	printf("sample count: %ld\n", samplecount);
	
	fread(&tmp32, 1, 4, infile);
	if(tmp32 != SACD_BLOCK_SIZE_PER_CHANNEL){
		printf("block size per channel %d is not %d\n", tmp32, SACD_BLOCK_SIZE_PER_CHANNEL);
		return 1;
	}
	
	fseek(infile, 92, SEEK_SET); //this offset is apparently fixed

	inbuf = malloc(channels*BUFFER_SAMPLES/8);
	outbuf = malloc(channels*BUFFER_SAMPLES);
	while(samplecount > 0){
		n = fread(inbuf, 1, channels*BUFFER_SAMPLES/8, infile);
		if(n < channels*BUFFER_SAMPLES/8){
			break;
		}
		for(i=0; i<channels; i++){
			for(j=0; j<BUFFER_SAMPLES/8; j++){
				tmp = inbuf[i*BUFFER_SAMPLES/8 + j];
				for(k=0; k<8; k++){
					outbuf[i + j*channels*8 + k*channels] = ((tmp & 0x01) << 7) | 0x40;
					tmp = tmp >> 1;
				}
			}
		}
		fwrite(outbuf, 1, n*8, outfile);
		samplecount -= BUFFER_SAMPLES;
	}
	fclose(infile);
	fclose(outfile);
	
	printf("DONE!\n");
	printf("When you load the pcm file, set channels = %d and sample format = 8bit unsigned/signed (doesn't matter for this program's output)\n", channels);
	printf("Set sample rate to %d if possible. If not, set it to anything you like and take this into account when reading timestamps and spectral displays\n", samplerate);
	
	return 0;
}
