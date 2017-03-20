// Doom WAD extractor
// first v 20 Mar 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
//
// * only extracts the music files at the moment. you can use a MUS2MIDI tool to
//   convert them to MIDIs http://iddqd.ru/utils?find=mus2midi

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <alloca.h> // this should be malloc.h or sthng on windows?

// HEADER stuff
char pwad_iwad[5];
int dir_nentries;
int dir_location; // integer holding pointer
// DIRECTORY stuff
// each entry has a lump metadata struct
typedef struct Lump {
	int location; // integer holding pointer
	int bytes;
	char name[9];
} Lump;
Lump* lumps;

int main( int argc, char** argv ) {
	printf( "De-WAD for Id Software WAD files. Anton Gerdelan\n" );
	if ( argc < 2 ){
		printf( "Call with name of WAD file. eg:\n./dewad DOOM1.WAD\n" );
		return 0;
	}
	FILE* f = fopen( argv[1], "rb" );
	if (!f) {
		fprintf( stderr, "ERROR opening `%s`\n", argv[1] );
		return 1;
	}
	{ // HEADER (12-bytes)
		size_t ritems = 0;
		ritems = fread( pwad_iwad, 1, 4,  f );
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
		
		lumps = (Lump*)malloc( sizeof(Lump) * dir_nentries );
		assert( lumps );

//		printf( "lump names =\n" );
		size_t ritems = 0;
		for ( int i = 0; i < dir_nentries; i++ ) {
			ritems = fread( &lumps[i].location, 4, 1, f );
			assert( ritems == 1 );
			ritems = fread( &lumps[i].bytes, 4, 1, f );
			assert( ritems = 1 );
			ritems = fread( lumps[i].name, 1, 8, f );
			assert ( ritems = 8 );
			lumps[i].name[8] = '\0';
//			printf("%s ", lumps[i].name );
		}
//		printf("\n");
	}
	// note: MUS is a compressed MIDI format created by Paul Radek ~1993
	// files are generated with a MIDI2MUS tool. note that song timing is not
	// stored so you gotta know it - Doom songs are all 140Hz
	{ // extract music files based on D_... prefix in name
		for ( int i = 0; i < dir_nentries; i++ ) {
			if ( lumps[i].name[0] == 'D' && lumps[i].name[1] == '_' ) {
				printf( "extracting music track '%s'...\n", lumps[i].name );
				int ret = fseek( f, (long)lumps[i].location, SEEK_SET );	
				assert( ret != -1 );

				unsigned char* block = alloca( lumps[i].bytes );
				assert( block );
				size_t ritems = fread( block, 1, lumps[i].bytes, f );
				assert( ritems == lumps[i].bytes );

				char filename[1024] = { 0 };
				sprintf( filename, "%s.mus", lumps[i].name );
				FILE* fout = fopen( filename, "wb" );
				assert( fout );
				ritems = fwrite( block, (size_t)lumps[i].bytes, 1, fout );
				assert( ritems == 1 );
				fclose( fout );

			}
		}//endfor entries
	}
	free( lumps );
	fclose( f );

	return 0;
}
