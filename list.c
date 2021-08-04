//
// Created by Ryan on 7/31/2021.
//

#include <stdlib.h>
#include <assert.h>
#include  <stdio.h>

#include "list.h"

List *createList(){

    List *newList = malloc(sizeof (List));
    newList->first_node = NULL;
    newList->size = 0;

    return newList;
}


/* Adds the new item to the end of the list */
void insert( List *list, int data){

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


/* Returns the first item in the list FIFO */
int getData(List *list){

    int result = -1;

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

void printList(List *list){

    Node *curr_node;

    printf("\n\n******************Printing the list...************\n");
    printf("List size: %d\n", list->size);
    curr_node = list->first_node;
    int item_number = 0;
    while(curr_node != NULL){

        printf("Item #%d : %d\n", item_number , curr_node->data);
        curr_node = curr_node->next_node;
        item_number++;
    }






}