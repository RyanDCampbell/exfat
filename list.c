/*-----------------------------------------
// NAME: Ryan Campbell
// STUDENT NUMBER: 7874398
// COURSE: COMP 3430, SECTION: A01
// INSTRUCTOR: Franklin Bristow
// ASSIGNMENT: assignment #4
//
// REMARKS: Implement a List data structure that
// employs a FIFO policy.
//-----------------------------------------*/
#include <stdlib.h>
#include <assert.h>
#include  <stdio.h>

#include "list.h"

/*------------------------------------------------------
// createList
//
// PURPOSE: Initializes and returns a new List data
// structure that uses a FIFO policy.
// OUTPUT PARAMETERS:
//     Returns a pointer to the newly allocated List
//------------------------------------------------------*/
List *createList(){

    List *newList = malloc(sizeof (List));
    newList->first_node = NULL;
    newList->size = 0;

    return newList;
}


/*------------------------------------------------------
// insert
//
// PURPOSE: Given a pointer to a List data structure,
// along with the data to insert (represented as an
// unsigned int), this method adds the data to the end
// of the list.  Using a FIFO strategy.
// INPUT PARAMETERS:
//    Takes in a pointer to a List along with unsigned
// int, which is the data to be added to the end of the
// List. Negative values are undefined in this structure.
//------------------------------------------------------*/
void insert( List *list, unsigned int data){

    Node *curr_node;
    /*If we are adding to an empty list*/
    if(list->first_node == NULL){
        list->first_node  = malloc(sizeof (Node));
        assert(list->first_node != NULL);

        list->first_node->data = data;
        list->first_node->next_node = NULL;

    }
    else {

        curr_node = list->first_node;
        while(curr_node->next_node != NULL) {
            curr_node = curr_node->next_node;
        }
        curr_node->next_node = malloc(sizeof (Node));
        assert(curr_node->next_node != NULL);

        curr_node = curr_node->next_node;
        curr_node->data = data;

        curr_node->next_node = NULL;
    }

    list->size++;
}


/*------------------------------------------------------
// getData
//
// PURPOSE: Given a pointer to a List, this method returns
// the first data item from the list, and does the appropriate
// upkeep, such as de-increment the number of data items in the
// List.
// INPUT PARAMETERS:
//    Takes in a pointer a List data structure.
// OUTPUT PARAMETERS:
//     Returns an unsigned int with the value of the data
// item removed from the list.  If the list is empty, returns -1
// as negatives are undefined in this structure.
//------------------------------------------------------*/
unsigned int getData(List *list){

    unsigned int result = -1;

    if(list->size == 1){
        result = list->first_node->data;
        list->first_node = NULL;
        list->size--;
    }
    else if(list->size > 1){
        result = list->first_node->data;
        list->first_node = list->first_node->next_node;
        list->size--;
    }

    return result;
}

/*------------------------------------------------------
// printList
//
// PURPOSE: Given a pointer to a List, this method traverses
// the list, printing the contents of each data item neatly to
// the display.
// INPUT PARAMETERS:
//    Takes in a pointer to a list data structure to print.
//------------------------------------------------------*/
void printList(List *list){

    Node *curr_node;

    printf("\n\n****************** Printing the list... ************\n");
    printf("List size: %d\n\n", list->size);
    curr_node = list->first_node;
    unsigned int item_number = 0;
    while(curr_node != NULL){

        printf("Item #%d : %d\n", item_number , curr_node->data);
        curr_node = curr_node->next_node;
        item_number++;
    }
    printf("\n****************** Finished printing the list ************\n");

}