#include "list.h"

struct Node_t
{
    int data;
    struct Node_t *next;
    struct Node_t *previous;
};

struct List_t
{
    Node head;
    Node tail;
    Node iterator;
    size_t size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

/**
*	listGetFirst: Sets the internal iterator (also called current node) to
*	the first node in the list.
*	Use this to start iterating over the list.
*	To continue iteration use listGetNext
*
* @param list - The list for which to set the iterator and return the first node
* @return
* 	NULL if a NULL pointer was sent or the list is empty.
* 	The first node in the list otherwise
*/
static Node listGetFirst(List list)
{
    if (list == NULL)
    {
        return NULL;
    }

    list->iterator = list->head;
    return list->iterator;
}

/**
*	listGetNext: Advances the list iterator to the next node and returns it.
* @param list - The list for which to advance the iterator
* @return
* 	NULL if reached the end of the list, or the iterator is at an invalid state
* 	or a NULL sent as argument
* 	The next node in the list in case of success
*/
static Node listGetNext(List list)
{
    if (list == NULL)
    {
        return NULL;
    }

    // check if the iterator is in an invalid state
    // (can happen if the getnext was called before getfirst...)
    if (list->iterator == NULL)
    {
        return NULL;
    }

    list->iterator = list->iterator->next;
    return list->iterator;
}

#define LIST_FOREACH(iterator, list) \
    for(Node iterator = listGetFirst(list) ; \
        iterator ;\
        iterator = listGetNext(list))


/**
* listFind: search a node that matches the to_find pointer using the comparePtr function
*   given by the user when the list is created.
* @param list - Target list.
* @param to_find - contain the string we need to find. 
*   (must be of the same type as the second parameter of the compare function)
* @return
*   NULL if a NULL sent as argument
* 	if exists, return the first Node containing the string. otherwise, return NULL.
*/
static Node listFind(List list, int to_find)
{
    if (list == NULL )
    {
        return NULL;
    }

    LIST_FOREACH(iterator, list)
    {
        if (iterator->data == to_find)
        {
            return iterator;
        }
    }

    return NULL;
}



/**
* listContains: searches the list for the actual specified node
* @param node - Target node.
* @return
* 	true if the node was found
*   false otherwise
*/
static bool listContains(List list, Node node)
{
    if (list == NULL)
    {
        return false;
    }
    LIST_FOREACH(iterator, list)
    {
        if (iterator == node)
        {
            return true;
        }
    }

    return false;
}

List listCreate()
{
    List new_list = malloc(sizeof(*new_list));
    if (new_list == NULL)
    {
        return NULL;
    }

    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->iterator = NULL;
    new_list->size = 0;
    pthread_mutex_init(&(new_list->mutex), NULL);
    pthread_cond_init(&(new_list->cond), NULL);

    return new_list;
}

void listDestroy(List list)
{
    if (list != NULL)
    {
        listClear(list);
        pthread_mutex_destroy(&(list->mutex));
        pthread_cond_destroy(&(list->cond));
        free(list);
        list = NULL;
    }
}

ListResult listAdd(List list, int data)
{
    if (list == NULL)
    {
        return LIST_NULL_ARGUMENT;
    }

    Node new_node = malloc(sizeof(*new_node));
    if (new_node == NULL)
    {
        return LIST_OUT_OF_MEMORY;
    }

    new_node->data = data;

    // insert the new node in the end of the list
    pthread_mutex_lock(&(list->mutex));
    
        new_node->next = NULL;
        new_node->previous = list->tail;
        list->tail = new_node;
        (list->size)++;

        if (new_node->previous != NULL) // if this is not the only element
        {
            new_node->previous->next = new_node;
        }
        else
        {
            list->head = new_node;
        }
        pthread_cond_signal(&(list->cond));

    pthread_mutex_unlock(&(list->mutex));

    return LIST_SUCCESS;
}

ListResult listEnqueue(List list, int data)
{
    return listAdd(list,data);
}

ListResult listRemove(List list, int to_remove)
{
    if (list == NULL)
    {
        return LIST_NULL_ARGUMENT;
    }

    int result = LIST_SUCCESS;
    pthread_mutex_lock(&(list->mutex));

        Node node = listFind(list, to_remove);
        
        if (node == NULL)
        {
            result = LIST_NODE_NOT_EXIST;
        }
        else
        {
            // in case this is the first element but not the last
            if (node->previous == NULL && node->next != NULL)
            {
                // remove the elemnt from the top of the list and make the next one top of the list
                node->next->previous = NULL;
                list->head = node->next;
                node->next = NULL;
            }
            // in case this is the last element but not the first
            else if (node->next == NULL && node->previous != NULL)
            {
                // remove the last element and make the previous be the last
                list->tail = list->tail->previous;
                node->previous->next = NULL;
                node->previous = NULL;
            }
            // in case this is the last and first element
            else if (node->previous == NULL && node->next == NULL)
            {
                list->tail = NULL;
                list->head = NULL;
            }
            // in case the element is in the middle
            else if (node->previous != NULL && node->next != NULL)
            {
                // remove the element from between two diffrent elements and make them point to each other
                node->previous->next = node->next;
                node->next->previous = node->previous;
                node->previous = NULL;
                node->next = NULL;
            }

            free(node);
            (list->size)--;
        }

    pthread_mutex_unlock(&(list->mutex));

    return result;
}

int listDequeue(List list)
{
    int res = -1;
    if(list == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&(list->mutex));
        while (list->size == 0) 
        {
            pthread_cond_wait(&(list->cond), &(list->mutex));
        }
        
        Node node = list->head;
        // in case this is the first element but not the last
        if (node->next != NULL)
        {
            // remove the elemnt from the top of the list and make the next one top of the list
            node->next->previous = NULL;
            list->head = node->next;
            node->next = NULL;
        }
        // in case this is the last and first element
        else
        {
            list->tail = NULL;
            list->head = NULL;
        }
        res = node->data;
        (list->size)--;
        free(node);
    pthread_mutex_unlock(&(list->mutex));

    return res;
}

ListResult listClear(List list)
{
    if (list == NULL)
    {
        return LIST_NULL_ARGUMENT;
    }

    pthread_mutex_lock(&(list->mutex));
        Node current_node = listGetFirst(list);
        while (current_node != NULL)
        {
            // remove the current element and move on to the next
            Node element_to_free = current_node;
            current_node = listGetNext(list);

            free(element_to_free);
        }

        list->head = NULL;
        list->tail = NULL;
        list->iterator = NULL;
        list->size = 0;
    pthread_mutex_unlock(&(list->mutex));

    return LIST_SUCCESS;
}

List listCopy(List list)
{
    if (list == NULL)
    {
        return NULL;
    }

    List list_copy = listCreate();
    if (list_copy == NULL)
    {
        return NULL;
    }

    pthread_mutex_lock(&(list->mutex));
        LIST_FOREACH(iterator, list)
        {
            ListResult result = listAdd(list_copy, iterator->data);
            if (result != LIST_SUCCESS)
            {
                pthread_mutex_unlock(&(list->mutex));
                listDestroy(list_copy);
                return NULL;
            }
        }

        pthread_mutex_lock(&(list_copy->mutex));
            list_copy->size = list->size;
        pthread_mutex_unlock(&(list_copy->mutex));

    pthread_mutex_unlock(&(list->mutex));

    return list_copy;
}


int listGetSize(List list)
{
    if (list == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&(list->mutex));
        int size = list->size;
    pthread_mutex_unlock(&(list->mutex));

    return size;
}