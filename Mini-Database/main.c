#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_create_Persona() {
    Persona *p = create_Persona("John", "Doe", "123 Street", 30);
    printf("Name: %s\nSurname: %s\nAddress: %s\nAge:%d\n", p->name, p->surname, p->address, p->age);
}

void test_insert_and_findByName() {
    Database *db = malloc(sizeof(Database));
    Persona *p = create_Persona("John", "Doe", "123 Street", 30);
    insert(db, p);
    Persona *found = findByName(db, "John");
    free_database(db);
}

void test_insert_and_findBySurname() {
    Database *db = malloc(sizeof(Database));
    Persona *p = create_Persona("John", "Doe", "123 Street", 30);
    insert(db, p);
    Persona *found = findBySurname(db, "Doe");
    free_database(db);
}

int main() {
    test_create_Persona();
    test_insert_and_findByName();
    test_insert_and_findBySurname();
    return 0;
}