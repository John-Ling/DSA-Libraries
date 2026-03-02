#include "stacks.h"

// implementation of the stack data structure using a singly linked list for learning purposes

// create stack out of array
Stack* s_create(void* arr[], const size_t n, const size_t typeSize)
{
    if (arr == NULL && n != 0)
    {
        return NULL;
    }

    Stack* stack = (Stack*)malloc(sizeof(Stack));
    stack->items = LibLinkedList.create(NULL, 0, typeSize);
    for (size_t i = 0; i < n; i++)
    {
        // insert at the head of the linked list
        if (s_push(stack, arr[i]) == EXIT_FAILURE)
        {
            s_free(stack, NULL);
            return NULL;
        }
    }
    return stack;
}

// takes a user defined function pointer for freeing memory
// passing NULL will use a basic default function that works for most applications
int s_free(Stack* stack, void (*free_item)(void*))
{
    LibLinkedList.free(stack->items, free_item);
    stack->items = NULL;
    free(stack);
    stack = NULL;
    return EXIT_SUCCESS;
}

// pushes an item onto the stack
int s_push(Stack* stack, const void* value)
{
    return LibLinkedList.insert(stack->items, value, 0);
}

int s_push_int(Stack* stack, int value)
{
    return s_push(stack, &value);
}

int s_push_flt(Stack* stack, float value)
{
    return s_push(stack, &value);
}

int s_push_dbl(Stack* stack, double value)
{
    return s_push(stack, &value);
}

int s_push_chr(Stack* stack, char value)
{
    return s_push(stack, &value);
}

int s_push_str(Stack* stack, const char* value)
{
    return s_push(stack, (void*)value);
}

// pop a value off the stack
// pass null to out if you don't need to receive the item
int s_pop(Stack* stack, void* out)
{
    if (s_empty(stack))
    {
        return EXIT_FAILURE;
    }

    if (out != NULL)
    {
        // set value
        memcpy(out, stack->items->head->value, stack->items->dataSize);
    }
    return LibLinkedList.delete(stack->items, 0, free);
}

int s_peek(Stack* stack, void* out)
{
    if (s_empty(stack) || out == NULL)
    {
        EXIT_FAILURE;
    }
    memcpy(out, stack->items->head->value, stack->items->dataSize);
    return EXIT_SUCCESS;
}

bool s_empty(Stack *stack)
{
    return stack->items == NULL || stack->items->head == NULL;
}

int s_print(Stack *stack, void (*print)(const void* value))
{
    return LibLinkedList.print(stack->items, print);
}

const struct LibStack_l LibStack = {
    .create = s_create,
    .push = s_push,
    .push_chr = s_push_chr,
    .push_dbl = s_push_dbl,
    .push_flt = s_push_flt,
    .push_int = s_push_int,
    .push_str = s_push_str,
    .print = s_print,
    .peek = s_peek,
    .pop = s_pop,
    .is_empty = s_empty,
    .free = s_free
};