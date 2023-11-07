#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define MAX_PAGES 1000

typedef struct Node {
struct Node* next;     // Pointer to the next Node in the chain
struct Node* prev;     // Pointer to the previous Node in the chain
unsigned int page;     // Page number
size_t size;           // Size of the memory block
int is_hole;           // Flag indicating if it's a hole (1) or process (0)
void* start;           // Start address of the memory block
void* end;             // End address of the memory block
void* virtual_address; // MeMS virtual address for this block
void* physical_address; // MeMS physical address for this block
} Node;

void* next_mems_virtual_address = (void*)1000;
void* next_mems_physical_address = (void*)1000;

int t=0;
int v=0;
int q=0;

Node* head = NULL;
void* mem_start = NULL;
Node* first = NULL;
int initial_memory = 0; // Flag to track if initial memory was allocated

void mems_init() {
head = NULL;
initial_memory = 0;
mem_start = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (mem_start == MAP_FAILED) {
printf("mmap Error");
exit(EXIT_FAILURE);
} 
else {
initial_memory = 1; 
}
}

void mems_finish() {
Node* c = head;
while (c) {
Node* next = c->next;
munmap(c, c->size);
c = next;
}    
}


void* mems_malloc(size_t size) {
if (size == 0) {
fprintf(stderr, " Invalid size Error \n");
return NULL;
}

Node* c = head;
while (c) {
if (c->is_hole && c->size >= size) {
c->is_hole = 0;
return (void*)((char*)c->virtual_address);
}
c = c->next;
}

if(4096-v<size){
void* allocated_memory = mmap(NULL, (size_t)4096-v, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
Node* new = (Node*)allocated_memory;
new->virtual_address = next_mems_virtual_address;
new->physical_address = (void*)((char*)new + (size_t)4096-v);
next_mems_virtual_address = (void*)((char*)next_mems_virtual_address + (size_t)4096-v);
next_mems_physical_address = (void*)((char*)next_mems_virtual_address + (size_t)4096-v);
new->size = (size_t)4096-v;
new->is_hole=1;
v=0;
if (head) {
new->next = head;
new->prev = NULL;
head->prev = new;
head=new;
}
}

size_t total_size = size;
v=v+size;
void* allocated_memory = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (allocated_memory == MAP_FAILED) {
printf(" mmap  Error");
return NULL;
}

Node* new = (Node*)allocated_memory;
new->size = total_size;
new->is_hole = 0;
new->virtual_address = next_mems_virtual_address;
new->physical_address = (void*)((char*)new + sizeof(Node));

if (head) {
new->next = head;
new->prev = NULL;
head->prev = new;
}
    
if(t==0){
first = new;
t=1;
}
head = new;
    
    
    
next_mems_virtual_address = (void*)((char*)next_mems_virtual_address + total_size);
next_mems_physical_address = (void*)((char*)next_mems_virtual_address + total_size);
return (void*)((char*)new->virtual_address);
    
}

void mems_free(void* ptr) {
Node* c = head;
while (c) {
if ((void*)((char*)c->virtual_address) == ptr) {
if (c->is_hole) {
fprintf(stderr, " Double-free attempt Error\n");
return;
}
c->is_hole = 1;
return;
}
c = c->next;
}
fprintf(stderr, "Error -> Invalid pointer or already freed\n");
}

void mems_print_stats() {
printf("\nMEMS SYSTEM STATS\n\n"); 
int k[MAX_PAGES] = {0};  
int w=0;
int t=0;
Node* c = first;
while (c) {
Node* subchain_start = c;
Node* subchain_end = c;

while (c && c->page == subchain_start->page) {
subchain_end = c;
if(c->is_hole){
break;
}
c = c->prev;
}   
       
printf("MAIN[%lu:%lu]-> ", (unsigned long)subchain_start->virtual_address, (unsigned long)subchain_end->virtual_address + subchain_end->size - 1);
c = subchain_start;
while (c && c->page == subchain_start->page) {
if (c->is_hole) {
if(c->next->is_hole){
c=c->prev;
continue;
}
if(c->prev->is_hole) {
c->size+=c->prev->size;
}
printf("H");
printf("[%lu:%lu] <-> ", (unsigned long)c->virtual_address, (unsigned long)c->virtual_address + c->size - 1);
t++;
k[w]=t;
w++;
t=0;
c = c->prev;
break;
} 
else {
printf("P");
printf("[%lu:%lu] <-> ", (unsigned long)c->virtual_address, (unsigned long)c->virtual_address + c->size - 1);
t++;
}
c = c->prev;
}
k[w]=t;
printf("NULL\n");
}

size_t total_pages = 0;
size_t unused_memory = 0;
size_t memory = 0;
c = head;
while (c) {
memory += c->size;
if (c->is_hole) {
unused_memory += c->size;
}
c = c->next;
}

printf("\nPages used: %lu\n", memory/4096+1);
printf("Space unused: %lu\n", unused_memory);
printf("Main Chain Length: %lu\n", memory/4096+1);

    
printf("Sub-chain Length array: [");
for (int i = 0; i < MAX_PAGES; i++) {
if ( k[i]==0) {
break;
}
if ( k[i+1]==0) {
k[i]++;
}
printf("%d, ", k[i]);
}
printf("]\n\n");
}


void* mems_get(void* v_ptr) {
Node* c = head;
while (c) {
void* start = (void*)((char*)c->virtual_address);
void* end = (void*)((char*)start + c->size);
if (v_ptr >= start && v_ptr < end) {
ptrdiff_t offset = (char*)v_ptr - (char*)c->virtual_address;
return (void*)((char*)c->physical_address + offset);
}
c = c->next;
}
fprintf(stderr, "Invalid pointer Error\n");
return NULL;
}
