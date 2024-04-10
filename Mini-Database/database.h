
// This represent a record in the only schema of this database
typedef struct Persona {
    char name[20];
    char surname[50];
    char address[100];
    int age;
} Persona;

// This is a node of an index that hold a string
typedef struct IndexNodeString {
    char * value;
    Persona * persona;
    struct IndexNodeString * left;
    struct IndexNodeString * right;
} IndexNodeString;

// This is a node of an index that hold an int
typedef struct IndexNodeInt {
    int value;
    Persona * persona;
    struct IndexNodeInt * left;
    struct IndexNodeInt * right;
} IndexNodeInt;

typedef union IndexNode
{
    IndexNodeString * string;
    IndexNodeInt * integer;
} IndexNode;



// A database hold a set of records and a set of indexes
typedef struct {
    IndexNodeString * name;
    IndexNodeString * surname;
    IndexNodeString * address;
    IndexNodeInt * age;
} Database;

// TODO implement the following methods
// The method return a Persona or NULL 

void insert(Database * database, Persona * persona);
Persona* findByName(Database * database, char * name);
Persona* findBySurname(Database * database, char * surname);
Persona* findByAddress(Database * database, char * address);
Persona* findByAge(Database * database, int age);


//Create a new person 
Persona * create_Persona(char* name, char* surname, char* address ,int age);

//Operations on IndexNodeInt
IndexNodeInt * create_tree_int(int value, Persona * persona);
void insert_inorder_int(IndexNodeInt * root, int value, Persona * persona);
void free_tree_int(IndexNodeInt* root);

//Operations on IndexNodeString
IndexNodeString * create_tree_string(char * value, Persona * persona);
void insert_inorder_string(IndexNodeString * root, char * value, Persona * persona);
void free_tree_string(IndexNodeString* root);


//Free the database
void free_database(Database * database);