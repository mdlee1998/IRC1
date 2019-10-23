// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
//
// typedef struct huffmanNode{
//   char character;
//   int freq;
//   huffmanNode left;
//   huffmanNode right;
// } huffmanNode;
//
// typedef struct huffmanTree{
//   int size = 0;
//   huffmanNode* nodes;
// } huffmanTree;
//
// void newNode(char c, int freq){
//   huffmanNode* node = (huffmanNode *)malloc(sizeof(huffmanNode));
//   node->character = c;
//   node->freq = freq;
//   node->left = NULL;
//   node->right = NULL:
// }
//
// void swapNodes(huffmanNode** a, huffmanNode** b){
//   huffmanNode* tmp = *a;
//   *a = *b;
//   *b = t;
// }

// C program for Huffman Coding
#// C program for Huffman Coding
#include <stdio.h>
#include <stdlib.h>

// This constant can be avoided by explicitly
// calculating height of Huffman Tree
#define MAX_TREE_HT 100


int parentCount = 0;
int childCount = 1;

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

	for (int i = 0; i < size; ++i)
		minHeap->array[i] = newNode(i+32, freq[i]);

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
	return extractMin(minHeap);
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

void arrayCodes(struct MinHeapNode* root, int arr[], int top, char **codes, char *treeString)

{
	int index = parentCount++ * 4;
	treeString[index] = parentCount;
	treeString[index + 1] = root->data;
	treeString[index + 2] = '\0';
	treeString[index + 3] = '\0';

	// Assign 0 to left edge and recur
	if (root->left) {
		treeString[index + 2] = ++childCount;

		arr[top] = 0;
		arrayCodes(root->left, arr, top + 1, codes, treeString);
	}

	// Assign 1 to right edge and recur
	if (root->right) {
		treeString[index + 3] = ++childCount;

		arr[top] = 1;
		arrayCodes(root->right, arr, top + 1, codes, treeString);
	}

	// If this is a leaf node, then
	// it contains one of the input
	// characters, print the character
	// and its code from arr[]
	if (isLeaf(root)) {
		char *code = (char*)malloc(50 * sizeof(char));
		for(int i = 0; i < top; i ++)
			code[i] = arr[i] + '0';
		codes[root->data - 32] = strdup(code);
		free(code);
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
	printf("\n%i\n\n", parentCount);
}

// Driver program to test above functions

// Driver program to test above functions
int createHuffmanTree(int freq[], int size, char **codes, char *treeString)
{
	HuffmanCodes(freq, size, codes, treeString);
	return parentCount;
}

struct MinHeapNode* recreateTree(char* treeString, int index){
	if(treeString[index] == '\0')
		return NULL;
		

	struct MinHeapNode* node = newNode(treeString[index+1], 0);

	if(treeString[index + 2] != '\0'){
			int newIndex = ((treeString[index + 2] - 1) * 4);
			node->left = recreateTree(treeString, newIndex);
	}
	if(treeString[index + 3] != '\0'){
		int newIndex = ((treeString[index + 3] - 1) * 4);
		node->right = recreateTree(treeString, newIndex);
	}

	return node;

}
