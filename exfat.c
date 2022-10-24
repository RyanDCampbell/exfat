
/*-----------------------------------------
// NAME: Ryan Campbell
//
// REMARKS: Implement a File System reader
// that supports the exfat file system. Implements
// support for 3 commands, info, list, and get.
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
#define ENTRY_SIZE 32   /* Bytes */

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
    uint8_t  entry_type;

    /* This is the index of the first cluster of the cluster chain
     * as the FAT describes (Look at the corresponding entry in the FAT,
     * to build the cluster chain) */
    uint32_t first_bitmap_cluster;
    uint64_t first_bitmap_cluster_data_length;

    /* bitmap of free clusters */

    /* uint8_t cluster size */

    char *ascii_volume_label;
    unsigned long free_space;

}exfat;
#pragma pack(pop)

/*------------------------------------------------------
// sectorsToBytes
//
// PURPOSE: Given a pointer to an exfat volume struct and
// an int of sectors desired, this method calculates the
// number of bytes used by x sectors.
// INPUT PARAMETERS:
//    Takes in a pointer to an exfat volume struct and
// an int of sectors desired.
// OUTPUT PARAMETERS:
//     Returns the number of bytes used by x sectors.
//------------------------------------------------------*/
int sectorsToBytes(exfat *volume, int number_of_sectors){

    assert(volume != NULL);
    assert(number_of_sectors >= 0);

    return ((0x1 << volume->sector_size) * number_of_sectors);
}

/*------------------------------------------------------
// clustersToBytes
//
// PURPOSE: Given a pointer to an exfat volume struct and
// an int of sectors desired, this method calculates the
// number of bytes used by x clusters.
// INPUT PARAMETERS:
//    Takes in a pointer to an exfat volume struct and
// an int of cluster desired.
// OUTPUT PARAMETERS:
//     Returns the number of bytes used by x clusters.
//------------------------------------------------------*/
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

    printf("\n*********** exFAT META DATA **************\n");
    printf("Volume Name: %s\n", volume->ascii_volume_label);
    printf("Volume Serial Number: %u\n", volume->serial_number);
    printf("Sector Size: %d\n", sectorsToBytes(volume, 1));

    printf("Cluster Size: %d sector(s), %d bytes\n", (0x1 << volume->cluster_size), clustersToBytes(volume, 1));
    printf("Cluster Count: %d\n", volume->cluster_count);

    printf("FAT offset: %d\n", volume->fat_offset);
    printf("FAT length %d\n", volume->fat_length);
    printf("Number of FATs: %d\n", volume->number_of_fats);

    printf("Root cluster: %d\n", volume->root_cluster);

    printf("First Bitmap Cluster: %d\n", volume->first_bitmap_cluster);
    printf("Cluster heap offset: %d\n", volume->cluster_heap_offset);

    printf("\nTemp file meta data:\n");
    printf("Entry Type: %d\n", volume->entry_type);

    printf("********************************************\n\n");
}

/* Calculate the Offset to the Cluster Heap + the Offset of the Root Cluster */
unsigned long rootDirectory(exfat *volume_data){

    return ((volume_data->cluster_heap_offset * sectorsToBytes(volume_data, 1)) +
            ((0x1<< volume_data->sector_size)*(0x1 << volume_data->cluster_size))*(volume_data->root_cluster - 2));
}

/*------------------------------------------------------
// numUnsetBits
//
// PURPOSE: Given an uint_32 value, this method counts the
// number of unset bits by performing a uniary & operation
// on the passed value, and 1. If the result == 0, we
// know the bit is unset.  We then bitshift and repeat to
// count the remaining bits.
// INPUT PARAMETERS:
//     Takes in an uint_32 value to count how many bits
// are unset.
// OUTPUT PARAMETERS:
//     Returns an unsigned int equal to the number of unset
// bits in the passed uint_32 value.
//------------------------------------------------------*/
unsigned int numUnsetBits(uint32_t value){

    int result = 0;

    for(int i = 0; i < 32; i++){

        if((value & 1) == 0){
            result ++;
        }
        value = value >> 1;
    }
    return result;
}

/*------------------------------------------------------
// buildClusterChain
//
// PURPOSE: Given a file descriptor to an exfat volume,
// along with a cluster head index to the cluster that
// is the first in cluster of a cluster chain to build.
// This method builds a list of cluster numbers that
// makeup a cluster chain for the desired index location.
// INPUT PARAMETERS:
//     Takes in a file descriptor to an exfat volume,
// along with an unsigned int that is the index of the
// first cluster index of the cluster chain to be built.
// OUTPUT PARAMETERS:
//     Returns aa pointer to the cluster chain List.
//------------------------------------------------------*/
List *buildFatClusterChain(int volume_fd, exfat *volume, unsigned int cluster_heap_index){

    // printf("\nBuilding cluster chain...\n");
    // printf("Cluster heap index: %u\n", cluster_heap_index);

    List *cluster_chain = malloc(sizeof (List));
    insert(cluster_chain, cluster_heap_index);

    unsigned int next_cluster = cluster_heap_index;
    unsigned int cluster_number = 0;

    unsigned long offset = volume->fat_offset * sectorsToBytes(volume, 1);
    lseek(volume_fd, offset, SEEK_SET);

    /* Seek passed FatEntry[0] and FatEntry[1] */
    lseek(volume_fd, 8, SEEK_CUR);

    while(next_cluster != -1){

        /* calculating the offset like this ensures that if the clusters are not contiguous,
         * the cluster chain will be built correctly */
        offset = volume->fat_offset * sectorsToBytes(volume, 1);
        offset += 4 * next_cluster;
        lseek(volume_fd, offset, SEEK_SET);

        read(volume_fd, (void *) &next_cluster, 4 );
        printf("Next Cluster: %d\n", next_cluster);

        /* Ensures the end of cluster chain marker is not added to the list */
        if(next_cluster != -1){
            insert(cluster_chain, next_cluster);
            cluster_number++;
        }
    }

    //printList(cluster_chain);
    return cluster_chain;
}

List *buildClusterChain(int volume_fd, exfat *volume, unsigned int cluster_heap_index){

    // printf("\nBuilding cluster chain...\n");
    // printf("Cluster heap index: %u\n", cluster_heap_index);

    List *cluster_chain = malloc(sizeof (List));
    insert(cluster_chain, cluster_heap_index);

    unsigned int next_cluster = cluster_heap_index;
    unsigned int cluster_number = 0;

    unsigned long offset = volume->fat_offset * sectorsToBytes(volume, 1);
    lseek(volume_fd, offset, SEEK_SET);

    /* Seek passed FatEntry[0] and FatEntry[1] */
    lseek(volume_fd, 8, SEEK_CUR);

    while(next_cluster != -1){

        /* calculating the offset like this ensures that if the clusters are not contiguous,
         * the cluster chain will be built correctly */
        offset = volume->fat_offset * sectorsToBytes(volume, 1);
        offset += 4 * next_cluster;
        lseek(volume_fd, offset, SEEK_SET);

        read(volume_fd, (void *) &next_cluster, 4 );
        //printf("Next Cluster: %d\n", next_cluster);

        /* Ensures the end of cluster chain marker is not added to the list */
        if(next_cluster != -1){
            insert(cluster_chain, next_cluster);
            cluster_number++;
        }
    }

    //printList(cluster_chain);
    return cluster_chain;
}


/*------------------------------------------------------
// calculateFreeSpace
//
// PURPOSE: Given a file descriptor to an exfat volume,
// along with a pointer to an exfat volume struct and
// a pointer to the bitmap cluster chain, this method
// calculated the number of free KB of memory left on the
// volume.
// INPUT PARAMETERS:
//     Takes in a file descriptor to an exfat volume,
// along with a pointer to an exfat volume struct and
// a pointer to the bitmap cluster chain.
// volume.
// OUTPUT PARAMETERS:
//     Returns an unsigned long equal to the number of free
// KB in the exfat volume.
//------------------------------------------------------*/
void calculateFreeSpace(int volume_fd, exfat *volume, List *bitmap_cluster_chain){

    printf("\n\nCalculating free space...\n\n");

    unsigned long total_unset_bits = 0;
    unsigned long offset =  (volume->cluster_heap_offset * sectorsToBytes(volume, 1));
    int current_cluster_bytes_left;

    unsigned int temp;

   // unsigned long offset = volume->fat_offset * sectorsToBytes(volume, 1);

    while(bitmap_cluster_chain->size > 0){

        current_cluster_bytes_left = clustersToBytes(volume, 1);

        /*Calculate the offset to the heap in number of bytes */
        offset =  (volume->cluster_heap_offset * sectorsToBytes(volume, 1));
        offset += (getData(bitmap_cluster_chain) * clustersToBytes(volume,1));

        lseek(volume_fd, (long) offset, SEEK_SET);

        while(current_cluster_bytes_left >= 4){

            read(volume_fd, &temp ,4);

            //printf("Temp: %x\n", temp);
            total_unset_bits += numUnsetBits(temp);

            temp = 0;
            current_cluster_bytes_left -= 4;
        }
    }

    volume->free_space = total_unset_bits * clustersToBytes(volume, 1)/KILOBYTE_SIZE;
    printf("\nFree Space KB: %lu\n\n", volume->free_space);
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
    printf("Free space: %lu KB\n", volume->free_space);
    printf("Cluster Size: %d sector(s), %d bytes\n", (0x1 << volume->cluster_size), clustersToBytes(volume, 1));

}

void commandGet(exfat *volume){ }

unsigned int offsetUpkeep(int volume_fd, exfat *volume, List *cluster_chain, unsigned int bytes_read){

    unsigned int bytes = bytes_read;
    unsigned long offset;

    if(bytes_read >= clustersToBytes(volume,1)){

        offset =  ((volume->cluster_heap_offset * sectorsToBytes(volume, 1)) +
                ((0x1<< volume->sector_size)*(0x1 << volume->cluster_size))*(getData(cluster_chain)-2));
        lseek(volume_fd, offset, SEEK_SET);
        bytes = 0;
    }
    return bytes;
}

void commandList(int volume_fd, exfat *volume){

    /* This will seek back to the directory entry after the bitmap allocation table entry */
    unsigned long offset = rootDirectory(volume);
    offset += ENTRY_SIZE * 2; /* Seeks past the volume Name and allocation bitmap entries */
    void *temp_label;

    unsigned int bytes_read = 0;

    uint8_t file_name_length;
    uint32_t file_first_cluster;
    uint16_t file_attributes;
    char *file_name;

    List *root = buildClusterChain(volume_fd, volume, volume->root_cluster);
    //printList(root);

    getData(root);  /* Pop the first cluster because we are already in it and won't be needing it */

    lseek(volume_fd, (long) offset, SEEK_SET);


    while(volume->entry_type != 0){
        bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read + 1);
        read(volume_fd, (void * ) &volume->entry_type, 1);
        bytes_read++;

        //printf("HEX %x\n", volume->entry_type);

        /* If it is a file */
        if(volume->entry_type == 0x85){
            /* File */

            lseek(volume_fd, 3, SEEK_CUR);
            bytes_read += 3;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            read(volume_fd, (void *) &file_attributes, 2);
            bytes_read += 2;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            lseek(volume_fd, 26, SEEK_CUR);
            bytes_read += 26;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            lseek(volume_fd, 3, SEEK_CUR);
            read(volume_fd, (void *) &file_name_length, 1);
            bytes_read += 4;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            lseek(volume_fd, 16, SEEK_CUR);
            read(volume_fd, (void *) &file_first_cluster, 4);
            bytes_read += 20;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            lseek(volume_fd, 10, SEEK_CUR);
            bytes_read += 10;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            /* Read in the unicode volume label, storing it in a temporary label.  Convert to unicode, update the exfat struct label and free the temp label */
            temp_label = malloc(sizeof (char)*31); /* Set to 31 due to 30 readable chars and + 1 for NULL terminator */
            assert(temp_label != NULL);

            read(volume_fd, (void *) temp_label, ENTRY_SIZE -2);
            file_name = unicode2ascii(temp_label, volume->label_length);
            bytes_read += ENTRY_SIZE -2;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            file_attributes = file_attributes >> 4;
            if(bytes_read > 0){
                if((file_attributes & 1) == 1){
                    printf("Directory: ");
                }
                else {
                    printf("File: ");
                }
                printf("%s\n", file_name);
            }
            free(temp_label);

        }
        else if(volume->entry_type == 0xc0){
            // printf("Stream extension\n");
            lseek(volume_fd, 2, SEEK_CUR);
            read(volume_fd, (void *) &file_name_length, 1);
            bytes_read += 3;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);
            lseek(volume_fd, 28, SEEK_CUR);
            // printf("File Name Length: %d\n", file_name_length);
        }
        else if(volume->entry_type == 0xc1){
           // printf("File Name\n");

            lseek(volume_fd, 1,SEEK_CUR);

            /* Read in the unicode volume label, storing it in a temporary label.  Convert to unicode, update the exfat struct label and free the temp label */
            temp_label = malloc(sizeof (char)*31); /* Set to 31 due to 30 readable chars and + 1 for NULL terminator */
            assert(temp_label != NULL);

            read(volume_fd, (void *) temp_label, ENTRY_SIZE -2);
            file_name = unicode2ascii(temp_label, volume->label_length);
            bytes_read += ENTRY_SIZE -2;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);

            file_attributes = file_attributes >> 4;
            if((file_attributes & 1) == 1){
                printf("Directory: ");
            }
            else {
                printf("File: ");

            }
            printf("%s\n", file_name);
            free(temp_label);
        }
        else{
            lseek(volume_fd, ENTRY_SIZE -1, SEEK_CUR);
            bytes_read += ENTRY_SIZE -1;
            bytes_read = offsetUpkeep(volume_fd, volume, root, bytes_read);
        }
    }
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

    exfat *volume = malloc(sizeof (exfat));
    List *bitmap_cluster_chain;
    unsigned long offset;
    void *temp_label;
    int curr_volume;

    assert(volume != NULL);

    if(volume != NULL){

        /* Read Boot Sector (first 512 bytes) */

        lseek(volume_fd, 80, SEEK_SET);
        read(volume_fd,(void *) &volume->fat_offset, 4);
        read(volume_fd, (void *) &volume->fat_length, 4);

        read(volume_fd, (void *) &volume->cluster_heap_offset, 4);
        read(volume_fd, (void *) &volume->cluster_count, 4);

        /* First cluster of the root directory located at offset 96 */
        read(volume_fd, (void *) &volume->root_cluster, 4);

        /* Volume serial number located at offset 100 */
        read(volume_fd, (void *) &volume->serial_number, 4);

        /* Volume cluster size located at offset 108 */
        lseek(volume_fd, 4, SEEK_CUR);
        read(volume_fd, (void *) &volume->sector_size, 1);
        read(volume_fd, (void *) &volume->cluster_size, 1);
        read(volume_fd, (void *) &volume->number_of_fats, 1);

        /* Calculate the Offset to the Cluster Heap + the Offset of the Root Cluster */
        offset = rootDirectory(volume);
        lseek(volume_fd, (long)offset, SEEK_SET);

        /* Read the length of the Volume Label */
        read(volume_fd, (void *) &volume->entry_type,1);
        read(volume_fd, (void *) &volume->label_length, 1);

        /* Read in the unicode volume label, storing it in a temporary label.  Convert to unicode, update the exfat struct label and free the temp label */
        temp_label = malloc(sizeof (char)*23); /* Set to 23 due to 22 readable chars and + 1 for NULL terminator */
        assert(temp_label != NULL);
        read(volume_fd, (void *) temp_label, 22);
        lseek(volume_fd, 8, SEEK_CUR);
        volume->ascii_volume_label = unicode2ascii(temp_label, volume->label_length);
        free(temp_label);

        read(volume_fd, (void *) &volume->entry_type,1);
        read(volume_fd, (void *) &curr_volume, 1);

        lseek(volume_fd, 18, SEEK_CUR);

        /* This is the index of the first cluster of the cluster chain
         * as the FAT describes (Look at the corresponding entry in the FAT,
         * to build the cluster chain) */
        read(volume_fd, (void *) &volume->first_bitmap_cluster, 4);
        read(volume_fd, (void *) &volume->first_bitmap_cluster_data_length, 8);

        /* Create and build the allocation bitmap table cluster chain */
        bitmap_cluster_chain = buildFatClusterChain(volume_fd, volume, volume->first_bitmap_cluster);

        /* Calculate and update the exfat struct volume to contain the number of KB free */
        calculateFreeSpace(volume_fd, volume, bitmap_cluster_chain);
    }
    return volume;
}

/*------------------------------------------------------
// main
//
// PURPOSE: this is the exfat file system reader's main
// that is run when the program is executed. It reads the
// arguments passed to the program by the user, looking to
// accept the name of the volume to read (including the .exfat
// extension) along with the command to run.  It then calls
// the appropriate methods to populate an exfat struct, and
// performed the desired command if it is supported.
//
// INPUT PARAMETERS:
//     Takes in arguments from standard I/O given by the
// user.  The program requires 2 arguments to be passed, the
// first argument is the name of the exfat volume to be read.
// The second argument is the name of the command that the
// user would like ot run.
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

        volume_fd = open(volume_name, O_RDONLY);

        if (volume_fd > 0) {
            printf("Opening file: '%s'\n", volume_name);

            if ((strcmp(command, "info") == 0) || (strcmp(command, "list") == 0) || (strcmp(command, "get") == 0)) {

                printf("Supported command");

                /* Able to open file and the command is valid, do work */

                volume = readVolume(volume_fd);

                displayMetadata(volume);

                if(strcmp(command, "info") == 0){
                    printf("Processing command: info...\n");
                    commandInfo(volume);
                }
                else if(strcmp(command, "list") == 0){
                    printf("Processing command: list...\n");
                    commandList(volume_fd, volume);
                }
                else if(strcmp(command, "get") == 0){
                    printf("Processing command: get...\n");
                    commandGet(volume);
                }

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
