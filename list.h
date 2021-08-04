//
// Created by Ryan on 7/31/2021.
//

#ifndef FSREADER_LIST_H
#define FSREADER_LIST_H

 typedef struct Node {

    int data;
    struct Node *next_node;

} Node ;

typedef struct List {

    Node *first_node;
    int size;

} List ;


List *createList();

void insert(List *list, int data);

int getData(List *list);

void printList(List *list);


#endif //FSREADER_LIST_H
