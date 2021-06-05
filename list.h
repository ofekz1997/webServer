#ifndef List_H_
#define List_H_


#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "bool.h"

/**
* List Container
*
* Implements a linked list container type.
* The type of the key and the value is generic (void *)
* The list has an internal iterator for external use. For all functions
* where the state of the iterator after calling that function is not stated,
* it is undefined. That is you cannot assume anything about it.
*
* The following functions are available:
*   listCreate		- Creates a new empty list with specified generic functions
*   listDestroy		- Deallocates all the nodes and frees the list.
*   listCopy		- Copies the data from each node and adds it to a new list
*   listGetSize		- Returns the number of nodes in the given list.
*   listFind    	- Searches the list for a specific node using the comparePtr
*                     (if found returns the first matching node)
*                     function specified when the list was created.
*   listAdd		    - Adds a new node to the start of the list with the given data.
*   listGet  	    - Returns the data from the specified node,
*					  Iterator status unchanged.
*   listRemove		- Removes the given node from the list.
*   listGetFirst    - Sets the internal iterator to the first node in the
*   				  list, and returns it.
*   listGetNext		- Advances the internal iterator to the next node and
*   				  returns it.
*	listClear		- Clears the contents of the list. Frees all the nodes of
*	 				  the list using the destroyPtr function given at the creation.
*   listGet         - Returns a pointer to the data of the given node.
*   listContains    - Checks if the given node is one of the lists nodes.
*
* 	LIST_FOREACH	- A macro for iterating over the list's nodes.
*/

/** Type used for returning error codes from functions */
typedef enum ListResult_t {
    LIST_SUCCESS,
    LIST_OUT_OF_MEMORY,
    LIST_NULL_ARGUMENT,
    LIST_NODE_NOT_EXIST
} ListResult;

typedef struct Node_t* Node;
typedef struct List_t* List;


/**
* listCreate: Allocates a new empty generic list.
* @return
* 	NULL - if allocations failed or one of the params is NULL.
* 	A new list in case of success.
*/
List listCreate();


/**
* listDestroy: Deallocates an existing list. Clears all elements.
*
* @param list - Target list to be deallocated. If list is NULL nothing will be
* 		done
*/
void listDestroy(List list);


/**
*	listAdd: add a node with the specified data to the list, a copy of the data will be added
*
* @param data - The data that the new node will contain
* @param list - The list to which to add the new data
* @return
*   LIST_NULL_ARGUMENT if one of the params is NULL
* 	LIST_OUT_OF_MEMORY if an allocation failed (Meaning the function for copying
* 	an element failed)
* 	LIST_SUCCESS if the element had been inserted successfully
*/
ListResult listAdd(List list, int data);

/**
*	listEnque: add a node with the specified data to the list, a copy of the data will be added
*
* @param data - The data that the new node will contain
* @param list - The list to which to add the new data
* @return
*   LIST_NULL_ARGUMENT if one of the params is NULL
* 	LIST_OUT_OF_MEMORY if an allocation failed (Meaning the function for copying
* 	an element failed)
* 	LIST_SUCCESS if the element had been inserted successfully
*/
ListResult listEnqueue(List list, int data);



/**
*  listRemove: Removes the given node from the list. Once found,
*  the nodes are removed and deallocated.
*  Iterator's value is undefined after this operation.
*
* @param list
*   The list from which to remove the node.
* @param node
* 	The node to remove from the list.
* @return
*   LIST_NULL_ARGUMENT if one of the params is NULL
*   LIST_NODE_NOT_EXIST if the list does not contain the given node
*   LIST_SUCCESS if the node was removed successfully
*/
ListResult listRemove(List list, int to_remove);

/**
*  listDequeue: Removes the first node from the list. Once found,
*  the nodes are removed and deallocated.
*  Iterator's value is undefined after this operation.
*
* @param list
*   The list from which to remove the node.
* @return the data of the node
*/
int listDequeue(List list);



/**
* listClear: Removes all data elements from target list.
* The data in each node is deallocated using the destroyPtr function specified upon creation of the list.
* Each node deallocated using free function.
* @param list
* 	Target list to remove all element from.
* @return
*   LIST_NULL_ARGUMENT if a NULL sent as argument
*   LIST_SUCCESS if the list was cleared successfully
*/
ListResult listClear(List list);

/**
* listCopy: Creates a copy of target list.
*   copies the data in each node using the copyPtr function specified upon creation of the list.
* @param list - Target list.
* @return
*   NULL if a NULL sent as argument or an allocation failed
* 	A list containing the same elements as list otherwise.
*/
List listCopy(List list);

/**
* listGetSize: Returens the number of nodes in a list.
* @param list - The list whose size is requested.
* @return
* 	-1 if a NULL pointer was sent.
* 	Otherwise the number of nodes in the list.
*/
int listGetSize(List list);



/**
* listGet: return pointer to the data in the specified node
* @param node - Target node.
* @return
* 	The actual pointer containing the data of the node.
*   NULL if the node is NULL
*/
int listGet(Node node);



#endif