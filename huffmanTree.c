
#include <stdio.h>
#include <stdlib.h>

#include "huffmanEncode.h"


//*************CODE TAKEN FROM https://www.geeksforgeeks.org/huffman-coding-greedy-algo-3/***************************************

#define MAX_TREE_HT 100


int treeStringIndex = 0;
int idx;
int cont;

// A Huffman tree node
struct MinHeapNode {

	// One of the input characters
	char data;

	// Frequency of the character
	unsigned freq;

	// Left and right child of this node
	struct MinHeapNode *left, *right;
};

// A Min Heap: Collection of
// min-heap (or Huffman tree) nodes
struct MinHeap {

	// Current size of min heap
	unsigned size;

	// capacity of min heap
	unsigned capacity;

	// Array of minheap node pointers
	struct MinHeapNode** array;
};

// A utility function allocate a new
// min heap node with given character
// and frequency of the character
struct MinHeapNode* newNode(char data, unsigned freq)
{
	struct MinHeapNode* temp
		= (struct MinHeapNode*)malloc
(sizeof(struct MinHeapNode));

	temp->left = temp->right = NULL;
	temp->data = data;
	temp->freq = freq;

	return temp;
}

// A utility function to create
// a min heap of given capacity
struct MinHeap* createMinHeap(unsigned capacity)

{

	struct MinHeap* minHeap
		= (struct MinHeap*)malloc(sizeof(struct MinHeap));

	// current size is 0
	minHeap->size = 0;

	minHeap->capacity = capacity;

	minHeap->array
		= (struct MinHeapNode**)malloc(minHeap->
capacity * sizeof(struct MinHeapNode*));
	return minHeap;
}

// A utility function to
// swap two min heap nodes
void swapMinHeapNode(struct MinHeapNode** a,
					struct MinHeapNode** b)

{

	struct MinHeapNode* t = *a;
	*a = *b;
	*b = t;
}

// The standard minHeapify function.
void minHeapify(struct MinHeap* minHeap, int idx)

{

	int smallest = idx;
	int left = 2 * idx + 1;
	int right = 2 * idx + 2;

	if (left < minHeap->size && minHeap->array[left]->
freq < minHeap->array[smallest]->freq)
		smallest = left;

	if (right < minHeap->size && minHeap->array[right]->
freq < minHeap->array[smallest]->freq)
		smallest = right;

	if (smallest != idx) {
		swapMinHeapNode(&minHeap->array[smallest],
						&minHeap->array[idx]);
		minHeapify(minHeap, smallest);
	}
}

// A utility function to check
// if size of heap is 1 or not
int isSizeOne(struct MinHeap* minHeap)
{

	return (minHeap->size == 1);
}

// A standard function to extract
// minimum value node from heap
struct MinHeapNode* extractMin(struct MinHeap* minHeap)

{

	struct MinHeapNode* temp = minHeap->array[0];
	minHeap->array[0]
		= minHeap->array[minHeap->size - 1];

	--minHeap->size;
	minHeapify(minHeap, 0);

	return temp;
}

// A utility function to insert
// a new node to Min Heap
void insertMinHeap(struct MinHeap* minHeap,
				struct MinHeapNode* minHeapNode)

{

	++minHeap->size;
	int i = minHeap->size - 1;

	while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {

		minHeap->array[i] = minHeap->array[(i - 1) / 2];
		i = (i - 1) / 2;
	}

	minHeap->array[i] = minHeapNode;
}

// A standard function to build min heap
void buildMinHeap(struct MinHeap* minHeap)

{

	int n = minHeap->size - 1;
	int i;

	for (i = (n - 1) / 2; i >= 0; --i)
		minHeapify(minHeap, i);
}

// A utility function to print an array of size n
void printArr(int arr[], int n)
{
	int i;
	for (i = 0; i < n; ++i)
		printf("%d", arr[i]);

	printf("\n");
}

// Utility function to check if this node is leaf
int isLeaf(struct MinHeapNode* root)

{

	return !(root->left) && !(root->right);
}

// Creates a min heap of capacity
// equal to size and inserts all character of
// data[] in min heap. Initially size of
// min heap is equal to capacity
struct MinHeap* createAndBuildMinHeap(int freq[], int size)
{

	struct MinHeap* minHeap = createMinHeap(size);

	int idx = 0;
	for (int i = 0; i < NUM_CHARS; ++i)
		if(freq[i] > 0){
			if(i == 94)
			minHeap->array[idx] = newNode(NL, freq[i]);
			else
				minHeap->array[idx] = newNode(i+32, freq[i]);
			idx++;
		}
	minHeap->array[idx] = newNode(FILEEND,1);
	minHeap->size = size;
	buildMinHeap(minHeap);

	return minHeap;
}


// The main function that builds Huffman tree
struct MinHeapNode* buildHuffmanTree(int freq[], int size)

{
	struct MinHeapNode *left, *right, *top;

	// Step 1: Create a min heap of capacity
	// equal to size. Initially, there are
	// modes equal to size.
	struct MinHeap* minHeap = createAndBuildMinHeap(freq, size);

	// Iterate while size of heap doesn't become 1
	while (!isSizeOne(minHeap)) {

		// Step 2: Extract the two minimum
		// freq items from min heap
		left = extractMin(minHeap);
		right = extractMin(minHeap);

		// Step 3: Create a new internal
		// node with frequency equal to the
		// sum of the two nodes frequencies.
		// Make the two extracted node as
		// left and right children of this new node.
		// Add this node to the min heap
		// '$' is a special value for internal nodes, not used
		top = newNode('$', left->freq + right->freq);

		top->left = left;
		top->right = right;

		insertMinHeap(minHeap, top);
	}

	// Step 4: The remaining node is the
	// root node and the tree is complete.
	struct MinHeapNode *root = extractMin(minHeap);
	free(minHeap);
	return root;
}

// Prints huffman codes from the root of Huffman Tree.
// It uses arr[] to store codes
void printCodes(struct MinHeapNode* root, int arr[], int top)

{

	// Assign 0 to left edge and recur
	if (root->left) {

		arr[top] = 0;
		printCodes(root->left, arr, top + 1);
	}

	// Assign 1 to right edge and recur
	if (root->right) {

		arr[top] = 1;
		printCodes(root->right, arr, top + 1);
	}

	// If this is a leaf node, then
	// it contains one of the input
	// characters, print the character
	// and its code from arr[]
	if (isLeaf(root)) {
		printf("%c: ", root->data);
		printArr(arr, top);

	}
}


//**************************************************************************************************
//*****EVERYTHING FOLLOWING IS EITHER MINE OR HEAVILY ALTERED***************************************


//Given the root of a huffman tree, will fill the codes array with the codes for each character
//working recursively.  Also builds the treestring
void arrayCodes(struct MinHeapNode* root, int arr[], int top, char **codes, char *treeString)

{


	if (isLeaf(root)) {
		char *code = (char*)calloc(25, sizeof(char));
		for(int i = 0; i < top; i ++)
			code[i] = arr[i] + '0';
		if(root->data == FILEEND)
			codes[95] = strdup(code);
		if(root->data == NL)
			codes[94] = strdup(code);
		else
			codes[root->data - 32] = strdup(code);
		free(code);

		treeString[treeStringIndex++] = '1';
		treeString[treeStringIndex++] = root->data;

		return;
	}

	treeString[treeStringIndex++] = '0';
	// Assign 0 to left edge and recur
	if (root->left) {
		arr[top] = 0;
		arrayCodes(root->left, arr, top + 1, codes, treeString);
	}

	// Assign 1 to right edge and recur
	if (root->right) {

		arr[top] = 1;
		arrayCodes(root->right, arr, top + 1, codes, treeString);
	}

	// If this is a leaf node, then
	// it contains one of the input
	// characters, print the character
	// and its code from arr[]

}

//Frees all the nodes in a huffman tree
void freeHuffmanTree(struct MinHeapNode *root){
	if(isLeaf(root))
		free(root);
	else{
		freeHuffmanTree(root->left);
		freeHuffmanTree(root->right);
	}
}

// The main function that builds a
// Huffman Tree and print codes by traversing
// the built Huffman Tree
void HuffmanCodes(int freq[], int size, char **codes, char *treeString)

{
	// Construct Huffman Tree
	struct MinHeapNode* root
		= buildHuffmanTree(freq, size);

	// Print Huffman codes using
	// the Huffman tree built above
	int arr[MAX_TREE_HT], top = 0;

	arrayCodes(root, arr, top, codes, treeString);
	freeHuffmanTree(root);
}

//Given an array of frequencies, will build a huffman tree, then fill a codes
//array with the codes for each character.  Will also build a treestring, to
//be added to an encoded file and allow rebuilding of the tree during decoding.
//Returns the number of characters in the treestring
int createHuffmanTree(int freq[], int size, char **codes, char *treeString)
{
	HuffmanCodes(freq, size, codes, treeString);
	return treeStringIndex;

}

//Given a tree string, will rebuild a Huffman Tree given a string recursively, using a
//pre-order traversal, where 0 represents an inner node, and 1 represents a leaf node,
//always followed by the character in said node
struct MinHeapNode* rebuild(char* treeString){

	struct MinHeapNode* root = newNode('$',0);
	cont--;

	if(cont == 0){
		return NULL;
	}

	if(treeString[idx - cont] == '0'){
		root->left = rebuild(treeString);
		root->right = rebuild(treeString);
	}else{
		cont--;
		root->data = treeString[idx - cont];
	}

	return root;
}

//Wrapper function to recreate a Huffman Tree given a string in decode
struct MinHeapNode* recreateTree(char* treeString, int thisCont, int thisIdx){
	idx = thisIdx;
	cont = thisCont;
	return rebuild(treeString);
}
