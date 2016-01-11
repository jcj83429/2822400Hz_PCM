#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define BUFFER_SAMPLES (44100*16)

// buf must be 5 or longer
void read_ID(FILE *infile, uint8_t *buf){
	if(fread(buf, 1, 4, infile) < 4){
		printf("file ended unexpectedly\n");
		exit(1);
	}
	buf[4] = 0;
}

uint64_t read_ckDataSize(FILE *infile){
	uint8_t tmp[8];
	if(fread(tmp, 1, 8, infile) < 8){
		printf("file ended unexpectedly\n");
		exit(1);
	}

	return (uint64_t) tmp[0]<<56 | (uint64_t) tmp[1]<<48 | (uint64_t) tmp[2]<<40 | (uint64_t) tmp[3]<<32 | (uint64_t) tmp[4]<<24 | (uint64_t) tmp[5]<<16 | (uint64_t) tmp[6]<<8 | (uint64_t) tmp[7];
}

int get_channel_count(FILE *infile, uint64_t datasize){
	uint64_t n = ftell(infile);
	uint64_t end = n + datasize;
	uint8_t ckID[5], tmp[2];
	uint64_t chunksize;
	int channelcount;

	while(n < end){
		read_ID(infile, ckID);
		chunksize = read_ckDataSize(infile);

		if(strcmp(ckID, "CHNL") == 0){
			if(fread(tmp, 1, 2, infile) < 2){
				printf("file ended unexpectedly\n");
				exit(1);
			}
			channelcount = tmp[0]<<8 | tmp[1];
			printf("channels = %d\n", channelcount);
			fseek(infile, chunksize-2, SEEK_CUR);
		}else if(strcmp(ckID, "CMPR") == 0){
			read_ID(infile, ckID);
			printf("compression type = %s\n", ckID);
			if(strcmp(ckID, "DSD ")){
				printf("compressed DSDIFF is not supported\n");
				exit(1);
			}
			fseek(infile, chunksize-4, SEEK_CUR);
		}else{
			//printf("%s chunk (skipped)\n", ckID);
			fseek(infile, chunksize, SEEK_CUR);
		}

		n = ftell(infile);
	}

	return channelcount;
}
	

int main(int argc, char *argv[]){
	FILE *infile, *outfile;
	int channelcount;
	uint8_t *inbuf, *outbuf;
	uint8_t tmp, ckID[5], tmparr[256];
	uint64_t i, j, k, n;

	uint64_t frm8size, chunksize, curpos, dsddatasize, inbufsize;

	if(argc != 3){
		printf("Usage: %s input.dff output.pcm\n", argv[0]);
		exit(1);
	}
	infile = fopen(argv[1], "rb");
	if(infile == NULL){
		printf("cannot open input\n");
		exit(1);
	}
	outfile = fopen(argv[2], "wb");
	if(outfile == NULL){
		printf("cannot open output\n");
		exit(1);
	}

	//params ok. parse global "form" chunk

	read_ID(infile, ckID);
	
	if(strcmp(ckID, "FRM8")){
		printf("not a valid DSDIFF file");
		exit(1);
	}

	frm8size = read_ckDataSize(infile);

	read_ID(infile, ckID);
	
	if(strcmp(ckID, "DSD ")){
		printf("not a valid DSDIFF file");
		exit(1);
	}

	//global form chunk ok. parse local chunks

	n = ftell(infile);

	while(n < 16 + frm8size){
		read_ID(infile, ckID);

		chunksize = read_ckDataSize(infile);

		if(strcmp(ckID, "PROP") == 0){
			printf("PROP chunk\n");
			read_ID(infile, ckID);
			if(strcmp(ckID, "SND ")){
				printf("PROP chunk is not of type SND , skipping");
				fseek(infile, chunksize-4, SEEK_CUR);
			}
			channelcount = get_channel_count(infile, chunksize-4);
		}else if(strcmp(ckID, "DSD ") == 0){
			printf("DSD  chunk\n");
			dsddatasize = chunksize;
			goto processdsddata;
		}else{
			//printf("%s chunk (skipped)\n", ckID);
			fseek(infile, chunksize, SEEK_CUR);
		}

		n = ftell(infile);
	}
	//WTF no DSD data found
	printf("FAIL: no DSD data!!\n");
	exit(1);

processdsddata:
	//necessary metadata parsed. process DSD data

	inbufsize = channelcount*BUFFER_SAMPLES/8;
	inbuf = malloc(inbufsize);
	outbuf = malloc(channelcount*BUFFER_SAMPLES);

	while(dsddatasize > 0){
		n = fread(inbuf, 1, (dsddatasize < inbufsize ? dsddatasize : inbufsize), infile);
		dsddatasize -= n;
		for(i=0; i<n/channelcount; i++){
			for(j=0; j<channelcount; j++){
				tmp = inbuf[i*channelcount + j];
				for(k=0; k<8; k++){
					outbuf[i*channelcount*8 + j + k*channelcount] = (tmp & 0x80) | 0x40; //0x40 centres the waveform
					tmp = tmp << 1;
				}
			}
		}
		fwrite(outbuf, 1, n*8, outfile);
	}

	fclose(infile);
	fclose(outfile);

	printf("DONE!\n");
	printf("When you load the pcm file, set channels = %d and sample format = 8bit unsigned/signed (doesn't matter for this program's output)\n", channelcount);
	printf("Set sample rate to %d if possible. If not, set it to anything you like and take this into account when reading timestamps and spectral displays\n", 44100*64);
	return 0;
}
