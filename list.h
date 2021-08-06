//
// Created by Ryan on 7/31/2021.
//

#ifndef FSREADER_LIST_H
#define FSREADER_LIST_H

 typedef struct Node {

    unsigned int data;
    struct Node *next_node;

} Node ;

typedef struct List {

    Node *first_node;
    unsigned int size;

} List ;


List *createList();

void insert(List *list, unsigned int data);

unsigned int getData(List *list);

void printList(List *list);


#endif //FSREADER_LIST_H
