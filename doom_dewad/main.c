// Doom WAD extractor
// first v 20 Mar 2017 Anton Gerdelan <antonofnote@gmail.com>
// C99
// DESCRIPTION: extracts all the sfx/music/images from Doom WAD
// COMPILE:     gcc -o dewad main.c -std=c99   (e.g. with GCC)
// RUN:         ./dewad DOOM1.WAD              etc.
// TODO:        Full Heretic/Hexen support. Export sfx as .wav. Individual map wads.
//
// Doom is a video game released by Id Software in 1993. Game data are stored in
// WADs - "Where's all the data?" - a play on BLOB for "Binary large object"
// It's an uncompressed blob with a quite simple internal format
// WAD files are either IWAD (internal) - main game data or PWAD (patch) - extras

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // Sean Barret's image writer
#include <alloca.h>					 // this should be malloc.h or sthng on windows?
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for convenience/safety
typedef unsigned char byte_t;
// for convenience
typedef struct rgb_t { byte_t r, g, b; } rgb_t;
// for type safety
typedef enum pal_t { PAL_DEFAULT = 0, PAL_MAX } pal_t;

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
// LUMP stuff
byte_t *palettes[PAL_MAX];

// for convenience/safety
rgb_t rgb_from_palette( byte_t colour_idx, pal_t pal ) {
	assert( colour_idx < 256 && colour_idx >= 0 );
	assert( pal < PAL_MAX && pal >= PAL_DEFAULT );

	int pal_idx = (int)pal;
	assert( palettes[pal_idx] );

	rgb_t rgb;
	rgb.r = palettes[pal_idx][colour_idx * 3];
	rgb.g = palettes[pal_idx][colour_idx * 3 + 1];
	rgb.b = palettes[pal_idx][colour_idx * 3 + 2];

	return rgb;
}

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

	{ // extract files based on prefix in name and other section markers
		// this was helpful
		// http://web.archive.org/web/20150421212726/http://aiforge.net/test/wadview/dmspec16.txt

		int nsounds_extracted = 0, nmusic_extracted = 0, ntextures_extracted = 0;
		// these categories are sectioned marked with start and end labels
		bool in_flats = false;
		bool in_map = false;

		for ( int dir_idx = 0; dir_idx < dir_nentries; dir_idx++ ) {
			// the last lump in map series
			if ( strncmp( "BLOCKMAP", lumps[dir_idx].name, strlen( "BLOCKMAP" ) ) == 0 ) {
				in_map = false;
				// printf("end map section\n");
				continue;
			}
			if ( in_map ) {
				continue;
			}
			// PLAYPAL - 14 palettes of RBA triples - 768 bytes each.
			// number 0 is the basic one. 10-12 are item pickup flash
			// 13 is radsuit. 3,2,0 are beserk red, number 'X' down to number '0' when
			// hurt
			if ( strncmp( "PLAYPAL", lumps[dir_idx].name, strlen( "PLAYPAL" ) ) == 0 ) {
				int ret = fseek( f, (long)lumps[dir_idx].location, SEEK_SET );
				assert( ret != -1 );

				// i think we can write them out as images
				for ( int palette_idx = 0; palette_idx < PAL_MAX; palette_idx++ ) {
					const size_t palette_sz = 768;
					palettes[palette_idx] = (byte_t *)malloc( palette_sz );
					assert( palettes[palette_idx] );

					size_t ritems = fread( palettes[palette_idx], 1, palette_sz, f );
					assert( ritems == palette_sz );

					char fname[1024];
					sprintf( fname, "palette%i.png", palette_idx );
					ret = stbi_write_png( fname, 16, 16, 3, palettes[palette_idx], 0 );
					assert( ret != 0 );
				}

				continue;
			}
			// COLORMAP - colour map levels for diminished lighting effect
			if ( strncmp( "COLORMAP", lumps[dir_idx].name, strlen( "COLORMAP" ) ) == 0 ) {
				continue;
			}
			// ENDDOOM - exit game text (MS DOS)
			if ( strncmp( "ENDOOM", lumps[dir_idx].name, strlen( "ENDOOM" ) ) == 0 ) {
				continue;
			}
			// DEMO1,2,3 - demos
			if ( strncmp( "DEMO", lumps[dir_idx].name, strlen( "DEMO" ) ) == 0 ) {
				continue;
			}
			// E1M1 etc - broad assumption here in name format
			if ( ( lumps[dir_idx].name[0] == 'E' && lumps[dir_idx].name[1] >= '0' &&
						 lumps[dir_idx].name[1] <= '9' && lumps[dir_idx].name[2] == 'M' &&
						 lumps[dir_idx].name[3] >= '0' && lumps[dir_idx].name[3] <= '9' ) ||
					 ( strncmp( "MAP", lumps[dir_idx].name, strlen( "MAP" ) ) == 0 ) &&
						 ( lumps[dir_idx].name[3] >= '0' && lumps[dir_idx].name[3] <= '9' ) &&
						 ( lumps[dir_idx].name[4] >= '0' && lumps[dir_idx].name[4] <= '9' ) ) {
				in_map = true;
				printf( "in map section `%s`\n", lumps[dir_idx].name );
				continue;
			}

			// TEXTURE1..3 - texture composition list
			if ( strncmp( "TEXTURE", lumps[dir_idx].name, strlen( "TEXTURE" ) ) == 0 ) {
				continue;
			}
			// PNAMES - wall patch numbers for names (indexing)
			if ( strncmp( "PNAMES", lumps[dir_idx].name, strlen( "PNAMES" ) ) == 0 ) {
				continue;
			}
			// GENMIDI - midi mapping
			if ( strncmp( "GENMIDI", lumps[dir_idx].name, strlen( "GENMIDI" ) ) == 0 ) {
				continue;
			}
			// DMXGUS - patch for Gravis Ultrasound
			if ( strncmp( "DMXGUS", lumps[dir_idx].name, strlen( "DMXGUS" ) ) == 0 ) {
				continue;
			}
			// DP* - PC speaker sounds (DOOM)
			if ( strncmp( "DP", lumps[dir_idx].name, strlen( "DP" ) ) == 0 ) {
				// 1 short 0
				// 1 short with num bytes of sound data (so 0 0 N 0 in little endian)
				// then that many bytes of sound data in some format
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
			if ( ( strncmp( "DS", lumps[dir_idx].name, 2 ) == 0 ) ) {
				const size_t header_sz = 8; // format stuff
				// NOTE: might need the padding to play properly...
				const size_t fronter_sz = 0; // 16; // padding
				const size_t ender_sz = 0;	 // 16; // padding

				//				printf( "extracting sfx '%s'...\n", lumps[dir_idx].name );

				int ret = fseek( f, (long)lumps[dir_idx].location + header_sz + fronter_sz,
												 SEEK_SET );
				assert( ret != -1 );

				byte_t *block = alloca( lumps[dir_idx].bytes );
				assert( block );
				size_t nbytes =
					(size_t)lumps[dir_idx].bytes - ( header_sz + fronter_sz + ender_sz );
				size_t ritems = fread( block, 1, nbytes, f );
				assert( ritems == nbytes );

				char filename[1024] = { 0 };
				sprintf( filename, "%s.raw", lumps[dir_idx].name );
				FILE *fout = fopen( filename, "wb" );
				assert( fout );
				ritems = fwrite( block, nbytes, 1, fout );
				assert( ritems == 1 );
				fclose( fout );

				nsounds_extracted++;
				continue;
			}

			// DOOM has D_E1M1 etc, HERETIC has MUS_...
			// note: MUS is a compressed MIDI format created ~1993 with proprietary engine
			// code from Paul Radek which is not incl in source code.
			// MUS files were generated with a MIDI2MUS tool. note that song timing is not
			// stored so you gotta know it - Doom songs are all 140Hz
			// MUS/DMX format is max 64kB big and max 9 channels
			if ( ( strncmp( "D_", lumps[dir_idx].name, 2 ) == 0 ) ||
					 ( strncmp( "MUS_", lumps[dir_idx].name, 4 ) == 0 ) ) {
				//				printf( "extracting music track '%s'...\n", lumps[dir_idx].name );
				int ret = fseek( f, (long)lumps[dir_idx].location, SEEK_SET );
				assert( ret != -1 );

				byte_t *block = alloca( lumps[dir_idx].bytes );
				assert( block );
				size_t ritems = fread( block, 1, lumps[dir_idx].bytes, f );
				assert( ritems == lumps[dir_idx].bytes );

				char filename[1024] = { 0 };
				sprintf( filename, "%s.mus", lumps[dir_idx].name );
				FILE *fout = fopen( filename, "wb" );
				assert( fout );
				ritems = fwrite( block, (size_t)lumps[dir_idx].bytes, 1, fout );
				assert( ritems == 1 );
				fclose( fout );

				nmusic_extracted++;
				continue;
			}

			if ( ( strncmp( "P_START", lumps[dir_idx].name, 7 ) == 0 ) ||
					 ( strncmp( "P1_START", lumps[dir_idx].name, 8 ) == 0 ) ||
					 ( strncmp( "P2_START", lumps[dir_idx].name, 8 ) == 0 ) ||
					 ( strncmp( "P3_START", lumps[dir_idx].name, 8 ) == 0 ) ) {
				printf( "start of patch lumps\n" );

				continue;
			}
			if ( ( strncmp( "P_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "P1_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "P2_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "P3_END", lumps[dir_idx].name, 5 ) == 0 ) ) {
				printf( "end of patch lumps\n" );

				continue;
			}
			if ( ( strncmp( "S_START", lumps[dir_idx].name, 7 ) == 0 ) ||
					 ( strncmp( "S1_START", lumps[dir_idx].name, 8 ) == 0 ) ||
					 ( strncmp( "S2_START", lumps[dir_idx].name, 8 ) == 0 ) ) {
				printf( "start of sprite lumps\n" );

				continue;
			}
			if ( ( strncmp( "S_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "S1_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "S2_END", lumps[dir_idx].name, 5 ) == 0 ) ) {
				printf( "end of sprite lumps\n" );

				continue;
			}

			// F_START -> F_END - "flats"" gfx section (F1_START for shareware)
			// flats are raw image data
			if ( ( strncmp( "F_START", lumps[dir_idx].name, 7 ) == 0 ) ||
					 ( strncmp( "F1_START", lumps[dir_idx].name, 8 ) == 0 ) ||
					 ( strncmp( "F2_START", lumps[dir_idx].name, 8 ) == 0 ) ||
					 ( strncmp( "F3_START", lumps[dir_idx].name, 8 ) == 0 ) ) {
				printf( "start of flats lumps\n" );
				in_flats = true;

				continue;
			}
			if ( ( strncmp( "F_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "F1_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "F2_END", lumps[dir_idx].name, 5 ) == 0 ) ||
					 ( strncmp( "F3_END", lumps[dir_idx].name, 5 ) == 0 ) ) {
				printf( "end of flats lumps\n" );
				in_flats = false;

				continue;
			}

			// each flat is 4096 bytes 64x64 raw pixels image
			// presumably indexing the DOOM palette
			if ( in_flats ) {
				// printf(" flats-%s\n", lumps[dir_idx].name);

				// look up palette RGB for each bytes and draw with stb_image
				const size_t flat_sz = 4096;
				byte_t *index_buff = alloca( flat_sz );
				assert( index_buff );
				byte_t *pixel_buff = alloca( flat_sz * 3 ); // rgb
				assert( pixel_buff );

				int ret = fseek( f, (long)lumps[dir_idx].location, SEEK_SET );
				assert( ret != -1 );
				size_t ritems = fread( index_buff, 1, flat_sz, f );
				assert( ritems == lumps[dir_idx].bytes );

				// use pixel in palette[0] (normal view) as o/p pixel RGB
				for ( int j = 0; j < flat_sz; j++ ) {
					rgb_t rgb = rgb_from_palette( index_buff[j], PAL_DEFAULT );
					pixel_buff[j * 3] = rgb.r;
					pixel_buff[j * 3 + 1] = rgb.g;
					pixel_buff[j * 3 + 2] = rgb.b;
				}

				// write image file
				char fname[1024];
				sprintf( fname, "%s.png", lumps[dir_idx].name );
				ret = stbi_write_png( fname, 64, 64, 3, pixel_buff, 0 );
				assert( ret != 0 );

				ntextures_extracted++;

				continue;
			}

			// all other lumps are other gfx in a custom format
			// name is 4-letters +  frame number 0...9..A...Z
			// + rotation key 0 for billboards or 1-8 for angles
			// better than unofficial spec: http://doom.wikia.com/wiki/Picture_format
			if ( !in_map && !in_flats ) {
				// printf("`%s`", lumps[dir_idx].name);

				// 8-byte HEADER of 4 short ints
				short int width = 0, height = 0, left_offset = 0, top_offset = 0;
				{
					int ret = fseek( f, (long)lumps[dir_idx].location, SEEK_SET );
					assert( ret != -1 );
					size_t sz = sizeof( short int );
					size_t ritems = fread( &width, sz, 1, f );
					assert( ritems == 1 );
					ritems = fread( &height, sz, 1, f );
					assert( ritems == 1 );
					ritems = fread( &left_offset, sz, 1, f );
					assert( ritems == 1 );
					ritems = fread( &top_offset, sz, 1, f );
					assert( ritems == 1 );
					printf( "%s w:%i h:%i lo:%i to:%i\n", lumps[dir_idx].name, width, height,
									left_offset, top_offset );
				}
				byte_t *pix_buff = (byte_t *)calloc( 4 * width * height, 1 );
				assert( pix_buff );

				// ptrs to data for each column
				// i guess width*sizeof(int) block here
				// these are offsets of each column from first byte of picture lump
				int *column_offsets = (int *)calloc( width, 4 );
				assert( column_offsets );
				size_t ritems = fread( column_offsets, 4, width, f );
				assert( ritems == width );

				// columns of bytes
				for ( int col_idx = 0; col_idx < width; col_idx++ ) {
					int ptr = column_offsets[col_idx];
					int lump_location = lumps[dir_idx].location;
					int ret = fseek( f, (long)( ptr + lump_location ), SEEK_SET );
					assert( ret != -1 );

					// row start being non-zero allows columns to be drawn skipping
					// transparent bits at the top
					while ( true ) {
						byte_t row_start = 0;
						// printf("%s\n", lumps[dir_idx].name);
						size_t ritems = fread( &row_start, 1, 1, f );
						assert( ritems == 1 );
						if ( row_start == 255 ) {
							break;
						}

						byte_t npixels_down = 0;
						ritems = fread( &npixels_down, 1, 1, f );
						assert( ritems == 1 );

						byte_t dead_byte = 0;
						ritems = fread( &dead_byte, 1, 1, f );
						assert( ritems == 1 );

						for ( int post_idx = 0; post_idx < npixels_down; post_idx++ ) {
							byte_t colour_idx = 0;
							ritems = fread( &colour_idx, 1, 1, f );
							assert( ritems == 1 );
							rgb_t rgb = rgb_from_palette( colour_idx, PAL_DEFAULT );
							int x = col_idx;
							int y = (int)row_start + post_idx;
							int pb_idx = width * y + x;

							if ( npixels_down > height ) {
								printf( "npixels_down %i height %i\n", npixels_down, height );
								assert( 0 );
							}

							pix_buff[pb_idx * 4] = rgb.r;
							pix_buff[pb_idx * 4 + 1] = rgb.g;
							pix_buff[pb_idx * 4 + 2] = rgb.b;
							pix_buff[pb_idx * 4 + 3] = 255;
						} // endfor down posts
						ritems = fread( &dead_byte, 1, 1, f );
						assert( ritems == 1 );
					} // endwhile rowstart
				}		// endfor across columns

				// write image file
				char fname[1024];
				sprintf( fname, "%s.png", lumps[dir_idx].name );
				int ret = stbi_write_png( fname, width, height, 4, pix_buff, 0 );
				assert( ret != 0 );

				free( column_offsets );
				free( pix_buff );

				ntextures_extracted++;

				continue;
			}

			printf( "WARNING: `%s` not a covered case\n", lumps[dir_idx].name );

		} // endfor entries

		printf( "palettes extracted: %i\n", (int)PAL_MAX );
		printf( "music files extracted: %i\n", nmusic_extracted );
		printf( "sound files extracted: %i\n", nsounds_extracted );
		printf( "images extracted: %i\n", ntextures_extracted );
	}
	for ( int pal_idx = 0; pal_idx < PAL_MAX; pal_idx++ ) {
		free( palettes[pal_idx] );
	}
	free( lumps );
	fclose( f );

	return 0;
}
