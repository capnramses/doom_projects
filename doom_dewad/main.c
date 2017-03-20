// Doom WAD extractor
// first v 20 Mar 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
// DESCRIPTION: extracts all the .MUS format music files out of a WAD file
// COMPILE:     gcc -o dewad main.c   (e.g. with GCC)
// RUN:         ./dewad DOOM1.WAD     (or heretic.wad, doom2.wad etc)
// TODO:        extract other types of files (images, sounds, etc).
//
// Doom is a video game released by Id Software in 1993. Game data are stored in
// WADs - "Where's all the data?" - a play on BLOB for "Binary large object"
// It's an uncompressed blob with a quite simple internal format
// WAD files are either IWAD (internal) - main game data or PWAD (patch) - extras

#include <alloca.h> // this should be malloc.h or sthng on windows?
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// HEADER stuff
char pwad_iwad[5]; // "IWAD" or "PWAD"
int dir_nentries;
int dir_location; // integer holding pointer
// DIRECTORY stuff
// each entry has a lump metadata struct
typedef struct Lump {
	int location; // integer holding pointer
	int bytes;
	char name[9];
} Lump;
Lump *lumps;

int main( int argc, char **argv ) {
	printf( "De-WAD for Id Software WAD files. Anton Gerdelan\n" );
	if ( argc < 2 ) {
		printf( "Call with name of WAD file. eg:\n./dewad DOOM1.WAD\n" );
		return 0;
	}

	FILE *f = fopen( argv[1], "rb" );
	if ( !f ) {
		fprintf( stderr, "ERROR opening `%s`\n", argv[1] );
		return 1;
	}
	{ // HEADER (12-bytes)
		size_t ritems = 0;
		ritems = fread( pwad_iwad, 1, 4, f );
		printf( "WAD type = `%s`\n", pwad_iwad );
		assert( ritems == 4 );
		ritems = fread( &dir_nentries, 4, 1, f );
		assert( ritems == 1 );
		printf( "dir entries = %i\n", dir_nentries );
		ritems = fread( &dir_location, 4, 1, f );
		assert( ritems == 1 );
		printf( "dir location = %i\n", dir_location );
	}
	{ // DIRECTORY (usually this is at end of WAD file)
		// SEEK_SET is offset from start of file. SEEK_CUR is from current fptr
		int ret = fseek( f, (long)dir_location, SEEK_SET );
		assert( ret != -1 );

		lumps = (Lump *)malloc( sizeof( Lump ) * dir_nentries );
		assert( lumps );

		//		printf( "lump names =\n" );
		size_t ritems = 0;
		for ( int i = 0; i < dir_nentries; i++ ) {
			ritems = fread( &lumps[i].location, 4, 1, f );
			assert( ritems == 1 );
			ritems = fread( &lumps[i].bytes, 4, 1, f );
			assert( ritems = 1 );
			ritems = fread( lumps[i].name, 1, 8, f );
			assert( ritems = 8 );
			lumps[i].name[8] = '\0';
			//			printf( "%s ", lumps[i].name );
		}
		//		printf("\n");
	}
	// note: MUS is a compressed MIDI format created ~1993 with proprietary engine
	// code from Paul Radek which is not incl in source code.
	// MUS files were generated with a MIDI2MUS tool. note that song timing is not
	// stored so you gotta know it - Doom songs are all 140Hz
	// MUS/DMX format is max 64kB big and max 9 channels
	{ // extract music files based on D_... prefix in name
		for ( int i = 0; i < dir_nentries; i++ ) {

			// DOOM has D_E1M1 etc, HERETIC has MUS_...
			if ( ( strncmp( "D_", lumps[i].name, 2 ) == 0 ) ||
					 ( strncmp( "MUS_", lumps[i].name, 4 ) == 0 ) ) {
				printf( "extracting music track '%s'...\n", lumps[i].name );
				int ret = fseek( f, (long)lumps[i].location, SEEK_SET );
				assert( ret != -1 );

				unsigned char *block = alloca( lumps[i].bytes );
				assert( block );
				size_t ritems = fread( block, 1, lumps[i].bytes, f );
				assert( ritems == lumps[i].bytes );

				char filename[1024] = { 0 };
				sprintf( filename, "%s.mus", lumps[i].name );
				FILE *fout = fopen( filename, "wb" );
				assert( fout );
				ritems = fwrite( block, (size_t)lumps[i].bytes, 1, fout );
				assert( ritems == 1 );
				fclose( fout );

				continue;
			}

			// all DOOM sounds are prefiex with 'DS' i think but heretic nope
			// might have to infer type by peeking or having a pre-made list of
			// sound effect label names
			// these are DMX format - also Paul Radek - which is raw 8-bit mono
			// at 11025Hz (one or two at 22050) w/ small header
			// support all 8 or 9 channels on early sound h/w + PC speaker o/p
			// format 0 -> pc speaker 1-2 are midi format sfx, 3 is digital
			// code was a bit unreliable. earlier versions supported random pitch
			// variation
			// some (all?) sounds have 16 lead and end padding bytes to allow v short
			// sounds to actually play using DMX software 
			if ( ( strncmp( "DS", lumps[i].name, 2 ) == 0 ) ) {
				const size_t header_sz = 8; // format stuff
				// NOTE: might need the padding to play properly...
				const size_t fronter_sz = 0;//16; // padding
				const size_t ender_sz = 0;//16; // padding
				
				printf( "extracting sfx '%s'...\n", lumps[i].name );

				int ret = fseek( f, (long)lumps[i].location + header_sz + fronter_sz, SEEK_SET );
				assert( ret != -1 );

				unsigned char *block = alloca( lumps[i].bytes );
				assert( block );
				size_t nbytes = (size_t)lumps[i].bytes - (header_sz + fronter_sz + ender_sz);
				size_t ritems = fread( block, 1, nbytes, f );
				assert( ritems == nbytes );

				char filename[1024] = { 0 };
				sprintf( filename, "%s.raw", lumps[i].name );
				FILE *fout = fopen( filename, "wb" );
				assert( fout );
				ritems = fwrite( block, nbytes, 1, fout );
				assert( ritems == 1 );
				fclose( fout );

				continue;
			}

		} // endfor entries
	}
	free( lumps );
	fclose( f );

	return 0;
}
