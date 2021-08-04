
/*-----------------------------------------
// NAME: Ryan Campbell
// STUDENT NUMBER: 7874398
// COURSE: COMP 3430, SECTION: A01
// INSTRUCTOR: Franklin Bristow
// ASSIGNMENT: assignment #4
//
// REMARKS: Implement a Task struct along with a
// Queue-like structure created out of an array
// of Task struct pointers.
//-----------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

#include <string.h>
#include <assert.h>

#include "list.h"

#define KILOBYTE_SIZE 1024

/*
 * The data heap itself is organized by clusters,
 * which may or may not be the same size as a sector.
 * Make sure that you’re using the right units in the
 * right regions! You should consider writing a utility
 * function or macro that quickly converts from cluster
 * number to bytes or from sector number to bytes.
 */

/* bitmap for allocation structure */


#pragma pack(push)
#pragma pack(1)
typedef struct EXFAT{

    uint32_t fat_offset;
    uint32_t fat_length;

    uint32_t cluster_heap_offset;  /* in # of sectors */
    uint32_t cluster_count;

    uint32_t root_cluster;
    uint32_t serial_number;

    /* 0x1 << cluster_size == cluster size in bytes */
    uint8_t sector_size;
    uint8_t cluster_size;

    uint8_t number_of_fats;

    uint8_t  label_length;
    uint16_t unicode_volume_label;
    char *ascii_volume_label;
    uint8_t  entry_type;

    /* This is the index of the first cluster of the cluster chain
 * as the FAT describes (Look at the corresponding entry in the FAT,
 * to build the cluster chain) */
    uint32_t first_bitmap_cluster;




    /* bitmap of free clusters */

    /* uint8_t cluster size */

}exfat;
#pragma pack(pop)


int sectorsToBytes(exfat *volume, int number_of_sectors){

    assert(volume != NULL);
    assert(number_of_sectors >= 0);

    return ((0x1 << volume->sector_size) * number_of_sectors);
}

int clustersToBytes(exfat *volume, int number_of_clusters){

    assert(volume != NULL);
    assert(number_of_clusters >= 0);

    return ((0x1<< volume->sector_size)*(0x1 << volume->cluster_size) * number_of_clusters);
}





/**
 * Convert a Unicode-formatted string containing only ASCII characters
 * into a regular ASCII-formatted string (16 bit chars to 8 bit
 * chars).
 *
 * NOTE: this function does a heap allocation for the string it
 *       returns, caller is responsible for `free`-ing the allocation
 *       when necessary.
 *
 * uint16_t *unicode_string: the Unicode-formatted string to be
 *                           converted.
 * uint8_t   length: the length of the Unicode-formatted string (in
 *                   characters).
 *
 * returns: a heap allocated ASCII-formatted string.
 */
static char *unicode2ascii( uint16_t *unicode_string, uint8_t length )
{
    assert( unicode_string != NULL );
    assert( length > 0 );

    char *ascii_string = NULL;

    if ( unicode_string != NULL && length > 0 )
    {
        /* +1 for a NULL terminator */
        ascii_string = calloc( sizeof(char), length + 1);

        if ( ascii_string )
        {
            /* strip the top 8 bits from every character in the */
            /* unicode string */
            for ( uint8_t i = 0 ; i < length; i++ )
            {
                ascii_string[i] = (char) unicode_string[i];
            }
            /* stick a null terminator at the end of the string. */
            ascii_string[length] = '\0';
        }
    }

    return ascii_string;
}


/*------------------------------------------------------
// commandInfo
//
// PURPOSE: Prints information about an exfat volume when
// the user enters the "info" command.  Information that is
// displayed includes; volume label, volume serial number,
// free space on volume in KB, and the cluster size in both
// sectors and in  KB.
// INPUT PARAMETERS:
//     Takes in a pointer to an exfat struct.
//------------------------------------------------------*/
void displayMetadata(exfat *volume){

    assert(volume != NULL);

    printf("FAT OFFSET: %d\n", volume->fat_offset);
    printf("Fat length %d\n", volume->fat_length);
    printf("Number of Fats: %d\n", volume->number_of_fats);
    printf("Volume Name: %s\n", volume->ascii_volume_label);
    printf("Entry Type: %d\n", volume->entry_type);
    printf("First Bitmap Cluster: %d\n", volume->first_bitmap_cluster);
    printf("Cluster Count: %d\n", volume->cluster_count);
    printf("Root cluster: %d\n", volume->root_cluster);
    printf("Cluster heap offset: %d\n", volume->cluster_heap_offset);
    printf("Volume Serial Number: %u\n", volume->serial_number);
    printf("Cluster Size: %d sector(s), %d bytes\n", (0x1 << volume->cluster_size), clustersToBytes(volume, 1));
    printf("Sector Size: %d\n", sectorsToBytes(volume, 1));

}

/*------------------------------------------------------
// commandInfo
//
// PURPOSE: Prints information about an exfat volume when
// the user enters the "info" command.  Information that is
// displayed includes; volume label, volume serial number,
// free space on volume in KB, and the cluster size in both
// sectors and in  KB.
// INPUT PARAMETERS:
//     Takes in a pointer to an exfat struct.
//------------------------------------------------------*/
void commandInfo(exfat *volume){

    assert(volume != NULL);

    printf("\n\nVolume label: %s\n", volume->ascii_volume_label);
    printf("Volume Serial Number: %u\n", volume->serial_number);
    printf("Free space: placeHolder\n");
    printf("Cluster Size: %d sector(s), %d bytes\n", (0x1 << volume->cluster_size), clustersToBytes(volume, 1));

//    printf("Entry Type: %d\n", volume->entry_type);
//    printf("Cluster Count: %d\n", volume->cluster_count);
//    printf("Root cluster: %d\n", volume->root_cluster);
//    printf("Cluster heap offset: %d\n", volume->cluster_heap_offset);
//    printf("Cluster Size: %d sector(s), %d bytes\n", (0x1 << volume->cluster_size), clustersToBytes(volume, 1));
//    printf("Sector Size: %d\n", sectorsToBytes(volume, 1));

}

/* Calculate the Offset to the Cluster Heap + the Offset of the Root Cluster */
unsigned long rootOffset(exfat *volume_data){

    return ((volume_data->cluster_heap_offset * sectorsToBytes(volume_data, 1)) +
            ((0x1<< volume_data->sector_size)*(0x1 << volume_data->cluster_size))*(volume_data->root_cluster - 2));
}




int numSetBits(uint32_t value){

    int result = 0;

    for(int i = 0; i < 32; i++){

        if((value & 1) == 1){
            result ++;
        }
        value = value >> 1;
    }
    return result;
}




/*------------------------------------------------------
// readVolume
//
// PURPOSE: Given a file descriptor to an exfat volume,
// this method allocates an exfat struct for the purpose of
// retrieving information from the volume. This method
// also populates the newly created exfat struct by reading
// the volume.
// INPUT PARAMETERS:
//     Takes in a file descriptor that points to an exfat
// volume.
// OUTPUT PARAMETERS:
//     Returns a pointer to the allocated exfat struct that
// was created by reading the volume pointed to the method
// byt the file descriptor.
//------------------------------------------------------*/
exfat *readVolume(int volume_fd){

    assert(volume_fd > 0);

    printf("\n\nReading the volume...\n\n");

    /* Volume label, Serial Number, Free Space, Cluster Size */

    exfat *volume_data = malloc(sizeof (exfat));
    unsigned long offset;
    void *temp_label;

    assert(volume_data != NULL);

    if(volume_data != NULL){

        /* Read Boot Sector (first 512 bytes) */

        lseek(volume_fd, 0, SEEK_SET);

        lseek(volume_fd, 80, SEEK_CUR);
        read(volume_fd,(void *) &volume_data->fat_offset, 4);
        read(volume_fd, (void *) &volume_data->fat_length, 4);
       // lseek(volume_fd, 88, SEEK_SET);

        read(volume_fd, (void *) &volume_data->cluster_heap_offset, 4);
        read(volume_fd, (void *) &volume_data->cluster_count, 4);

        /* First cluster of the root directory located at offset 96 */
        //lseek(volume_fd, 96, SEEK_CUR);
        read(volume_fd, (void *) &volume_data->root_cluster, 4);

        /* Volume serial number located at offset 100 */
        read(volume_fd, (void *) &volume_data->serial_number, 4);

        /* Volume cluster size located at offset 108 */
        lseek(volume_fd, 4, SEEK_CUR);
        read(volume_fd, (void *) &volume_data->sector_size, 1);
        read(volume_fd, (void *) &volume_data->cluster_size, 1);

        read(volume_fd, (void *) &volume_data->number_of_fats, 1);


        /* Calculate the Offset to the Cluster Heap + the Offset of the Root Cluster */
        offset = rootOffset(volume_data);
        /* Seek to the offset */
        lseek(volume_fd, (long)offset, SEEK_SET);







        /* Read the length of the Volume Label */
        //lseek(volume_fd, 1, SEEK_CUR);
        read(volume_fd, (void *) &volume_data->entry_type,1);
        //printf("\n\nHEX: %x\n\n", volume_data->entry_type);

        read(volume_fd, (void *) &volume_data->label_length, 1);

        /* Read in the unicode volume label, storing it in a temporary label.  Convert to unicode, update the exfat struct label and free the temp label */
        temp_label = malloc(sizeof (char)*23); /* Set to 23 due to 22 readable chars and + 1 for NULL terminator */
        assert(temp_label != NULL);
        read(volume_fd, (void *) temp_label, 22);
        lseek(volume_fd, 8, SEEK_CUR);

        volume_data->ascii_volume_label = unicode2ascii(temp_label, volume_data->label_length);
        free(temp_label);

        // In position to read the next directoryEntry.


        int volume;

        read(volume_fd, (void *) &volume_data->entry_type,1);
        read(volume_fd, (void *) &volume, 1);

//        printf("\n\nHEX: %x\n\n", volume_data->entry_type);
//        printf("\n\nShould be 0: %d\n\n", volume);

        lseek(volume_fd, 18, SEEK_CUR);

        /* This is the index of the first cluster of the cluster chain
         * as the FAT describes (Look at the corresponding entry in the FAT,
         * to build the cluster chain) */
        read(volume_fd, (void *) &volume_data->first_bitmap_cluster, 4);



        offset = volume_data->fat_offset * sectorsToBytes(volume_data, 1);


        //printf("Offset:%ld\n", offset);

        lseek(volume_fd, offset, SEEK_SET);
        lseek(volume_fd, 8, SEEK_CUR);

        /* Create and build the allocation bitmap table cluster chain */

        List *bitmap_cluster_chain = malloc(sizeof (List));
        int next_cluster;
        int cluster_number = 0;
        while(next_cluster != -1){
            read(volume_fd, (void *) &next_cluster, 4 );

            /* Ensures the end of cluster chain marker is not added to the list */
            if(next_cluster != -1){
                insert(bitmap_cluster_chain, next_cluster);
                cluster_number++;
            }
        }

        //printList(bitmap_cluster_chain);



    }
    return volume_data;
}








/*------------------------------------------------------
// main
//
// PURPOSE: this is the scheduler's main that is run when the
// scheduler is executed. It reads the arguments passed to the
// program by the user, looking to accept the number of desired
// CPUs to utilize, along with which scheduling policy to employ.
// it then reads the tasks from the task file, and sets up an
// array of structs of type Task.
//
// INPUT PARAMETERS:
//     Takes in arguments from standard I/O given by the
// user.  The program requires 2 arguments to be passed, the
// first argument is the desired number of CPUs to utilize
// in the simulation.  The second argument is which scheduling
// policy to employ.  The 2 options of scheduling policies are:
// sjf, and mlfq.
// OUTPUT PARAMETERS:
//     Returns 0 if the program finishes successfully.
//------------------------------------------------------*/
int main(int argc, char *argv[]) {

    char *command;
    char *volume_name;
   // char full_path[30];
    int volume_fd;
    exfat *volume;

    /* Ensure the user passes 2 parameters to the scheduler (Number of CPUs and scheduler type) */
    if (argc == 3) {

        volume_name = argv[1];
        command = argv[2];

        printf("\n\nReading Volume: %s, Command: %s\n", volume_name, command);

//        strcpy(full_path, "/1sector-per-cluster/");
//        strcat(full_path, volume_name);

//        printf("Full path: %s\n", full_path);
//

        volume_fd = open(volume_name, O_RDONLY);

        if (volume_fd > 0) {
            printf("Opening file: '%s'\n", volume_name);

            if ((strcmp(command, "info") == 0) || (strcmp(command, "list") == 0) || (strcmp(command, "get") == 0)) {

                printf("Supported command");

                /* Able to open file and the command is valid, do work */

                volume = readVolume(volume_fd);

                displayMetadata(volume);

                commandInfo(volume);

            } else {
                printf("Unsupported command");
            }

        } else {
            printf("Unable to open file: '%s'\n", volume_name);
        }

    } else {
        printf("\nPlease provide the program the volume to read, along with the command to run.\n\n\nExample: ./exfat volumeName info\n");
    }

    printf("\nProgram completed normally.\n\n");
    return EXIT_SUCCESS;
}