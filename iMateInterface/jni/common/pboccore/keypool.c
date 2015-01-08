/**************************************
File name     : keypool.c
Function      : RSA密钥缓冲池
Author        : Yu Jun
First edition : Mar 25th, 2011
Modified      : 
**************************************/

#include "keypool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include "r_keygen.h"

#ifdef UNIX
#include <unistd.h>
#include <pthread.h>
//#include <termio.h>
#include <libgen.h>

//#include "common.h"
//#include "reslib.h"
//#include "func.h"
#include "r_keygen.h"
//#include "common.h"
//#include "func.h"
//#include "reslib.h"

#else  //On Windows
#include <windows.h>
#include <tchar.h>
#endif // UNIX


#include "rsaref.h"
#include "nn.h"


//容器的默认容量
//#define DEF_CAPACITY 1000 
#define DEF_CAPACITY 100

#define DEF_MORE_NUM 20   
#define DEF_LESS_NUM 2    

#define DEF_E1   3
#define DEF_E2   65537

#ifdef DEBUG
#define PRINTT_SIZE(primePairPool)	do {			\
	printf("size(s:%d,rs:%d,d:%d,rd:%d)\n",			\
	       (primePairPool)->staticList->size,		\
	       (primePairPool)->r_staticList->size,		\
	       (primePairPool)->dynamicList->size,		\
	       (primePairPool)->r_dynamicList->size);		\
	fflush(stdout);						\
    }while(0);
#else
#define PRINTT_SIZE(primePairPool)
#endif //DEBUG

#define MIN_RSA_KEY_LEN     64
#define MAX_RSA_KEY_LEN     256

/***list 的定义***/

typedef int (*List_NodeDataCmp) (void *d1, void *d2);

typedef struct NodeTag Node;
struct NodeTag {
    Node   *prev;
    Node   *next;
    void   *data;
};

typedef struct ListTag List;
struct ListTag {
    Node              *head;
    Node              *tail;
    unsigned int       size;
    List_NodeDataCmp   NodeDataCmp;  //If the d1 and d2 matched, then return 0.
};

/* SubList is a part of List, and which is MUST belonged to a List. */
typedef struct SubListTag SubList;
struct SubListTag {
    Node *firstNode;
    Node *lastNode;
    unsigned int   size;
};

//线程控制函数
#ifdef UNIX
static void *PrimePool_ThreadFunc(void *arg);
#else  //Windows
static DWORD WINAPI PrimePool_ThreadFunc(LPVOID arg);
#endif //UNIX


/*
 * Methods about Node
 */
Node *Node_New(void *data);

/*
 * Methods about List
 */

//List constructor
static List    *List_New();

//Push the node in front of the list.
static void     List_PushFront(List *list, Node *node);

//Pop the first node of the list, and return the pointer of the node, or 0 for error.
static Node    *List_PopFront(List *list);

//Push the node append the list.
static void     List_PushBack(List *list, Node *node);

//Pop the last node of the list, and return the pointer of the node, or 0 for error.
static Node    *List_PopBack(List *list);

//Move fromNode before the toNode.
static void     List_MoveNodeBeforeNode(List *list, Node *fromNode, Node *toNode);

#if 0
//Move fromNode after the toNode.
static void     List_MoveNodeAfterNode(List *list, Node *fromNode, Node *toNode);

//Move node to the list's header, and node MUST belongs to list.
static void     List_MoveNodeToHead(List *list, Node *node);
#endif //#if 0

//Move subList to the list's header, and node MUST belongs to list.
static void     List_MoveSubListToHead(List *list, SubList *subList);

//Move the node to the subList, and append to its lastNode.
//Note: Both of node and subList MUST belong to the list.
static void     List_AppendNodeToSubList(List *list, Node *node, SubList *subList);

//Get all the nodes which matched node in the list together, and mark with subList;
static void     List_SortByData(List *list, void *data, SubList *subList);

/*
 * Methods about SubList
 */


#if 0
//Create a new subList
static SubList *SubList_New();

//Destroy the sublist
static void SubList_Destroy(SubList *subList);
#endif //#if 0: Unused Method
 
static void SubList_Init(SubList *);

/***^^^^^^list 的定义^^^^^^***/

// rsaKeyE    : 随机加密密钥，但通常是3或者65537
// rsaKeyLen  : RSA密钥长度(byte)，在PBOC2.0中，是[128byte,248byte]范围内的偶数
// p          : 素数因子p
// q          : 素数因子q
// 其中，p > q

typedef struct PrimePairTag PrimePair;
struct PrimePairTag {
    NN_DIGIT       rsaKeyE;
    unsigned long  rsaKeyLen;
    NN_DIGIT p[MAX_NN_DIGITS];
    NN_DIGIT q[MAX_NN_DIGITS];
};

typedef struct PrimePairPoolTag PrimePairPool;
struct PrimePairPoolTag  {
    List            *r_staticList;      //固定请求的素数对列表
    List            *staticList;        //可用的素数列表，数目固定，用一还一

    List            *r_dynamicList;     //动态请求的素数对列表
    List            *dynamicList;       //可用的素数对列表，根据使用热度调整

    unsigned int     dynamicCapacity;   //动态缓冲池容量

#ifdef UNIX
    pthread_t               tid;               //线程的tid
    pthread_mutex_t         mutex;             //缓冲池访问互斥锁
#else //On Windows
    HANDLE                  handle;            //线程的handle
	DWORD                   tid;               //线程的tid
	HANDLE                  mutex;             //缓冲池访问互斥锁
#endif //UNIX
};



static PrimePairPool *sg_primePairPool = 0;

// 素数池初始化
static PrimePairPool *PrimePool_New();

// 开始运行缓冲池线程
static void PrimePool_Start(PrimePairPool *primePairPool);

// 生成RSA密钥对
// in  : iRsaKeyLen  : rsa密钥长度
//       lRsaKeyE    : rsa密钥e值，3 or 65537
// out : pPublicKey  : 公钥
//       pPrivateKey : 私钥
// ret : 0 : OK
//       1 : 错误
static int PrimePool_GetKey(PrimePairPool *primePairPool, unsigned int rsaKeyLen, long int rsaKeyE,
		     R_RSA_PUBLIC_KEY *pPublicKey, R_RSA_PRIVATE_KEY *pPrivateKey);

static void PrimePair_Reste(PrimePair *rsaPrimePair, long int rsaKeyE, unsigned int rsaKeyLen);
static void PrimePair_Update(PrimePair *rsaPrimePair, long int rsaKeyE, unsigned int rsaKeyLen);
static int  PrimePair_Cmp(void *d1, void *d2);

static short
PrimePair_GeneratePEMKeys(PrimePair *primePair, R_RSA_PUBLIC_KEY *publicKey, R_RSA_PRIVATE_KEY *privateKey);

static void  PrimePool_Request(PrimePairPool *primePairPool, long int rsaKeyE,
			       unsigned int rsaKeyLen, int num);

static PrimePair *PrimePair_New(long int rsaKeyE, unsigned int rsaKeyLen);
static void PrimePair_Init(PrimePair *primePair);
static int  PrimePair_Cmp(void *d1, void *d2);

static void PrimePair_GenPrimes(PrimePair *rsaPrimePair);

static void PrimePairPool_ProcessRequest(PrimePairPool *primePairPool);

static void PrimePairPool_Lock(PrimePairPool *primePairPool);
static void PrimePairPool_Unlock(PrimePairPool *primePairPool);

static int  sg_iUseKeyPoolFlag = 0; // 1:use key pool 0:do not use key pool

// 密钥池初始化
// in  : iUseKeyPoolFlag : 1:启用Pool 0:不启用Pool
// ret : 0 : OK
//       1 : 错误
// Note: 即使初始化返回错误，后续依然可以调用iRsaKeyPoolGetKey()生成密钥
int iRsaKeyPoolInit(int iUseKeyPoolFlag)
{
	sg_iUseKeyPoolFlag = iUseKeyPoolFlag;
	if(sg_iUseKeyPoolFlag == 0)
		return(0);
	if(sg_primePairPool != 0)   //线程已经启动
		return 0;
    sg_primePairPool = PrimePool_New();
	if(sg_primePairPool == 0)  //内存分配错误
		return 1;
    PrimePool_Start(sg_primePairPool);
    return 0;
}

// 生成RSA密钥对
// in  : iRsaKeyLen  : rsa密钥长度
//       lRsaKeyE    : rsa密钥e值，3 or 65537
// out : pPublicKey  : 公钥
//       pPrivateKey : 私钥
// ret : 0 : OK
//       1 : 错误
int iRsaKeyPoolGetKey(int iRsaKeyLen, long lRsaKeyE, R_RSA_PUBLIC_KEY *pPublicKey, R_RSA_PRIVATE_KEY *pPrivateKey)
{
	R_RSA_PROTO_KEY   ProtoKey;
	int iRet;
	if(sg_iUseKeyPoolFlag == 0) {
		// 不使用KeyPool
		ProtoKey.bits = iRsaKeyLen * 8;
		ProtoKey.useFermat4 = lRsaKeyE == 3 ? 0 : 1;
	    iRet = R_GeneratePEMKeys(pPublicKey, pPrivateKey, &ProtoKey);
		return(iRet);
	}

    return  (int)PrimePool_GetKey(sg_primePairPool, iRsaKeyLen, lRsaKeyE, pPublicKey, pPrivateKey);
}



/***list 的实现***/

Node *Node_New(void *data)
{
    Node *node = (Node *) malloc(sizeof(Node));
    node->prev = 0;
    node->next = 0;
    node->data = data;
    return node;
}

List *List_New() 
{
    List *list = (List *) malloc(sizeof(List));
    list->head = 0;
    list->tail = 0;
    list->size = 0;
    list->NodeDataCmp = 0;
    return list;
}

void  List_PushFront(List *list, Node *node)
{
    node->next = list->head;
    node->prev = 0;
    if(list->head)
	list->head->prev = node;
    list->head = node;

    if(list->tail == 0)
	list->tail = node;
    ++(list->size);
}

Node *List_PopFront(List *list)
{
	Node *node = 0;
    if(list->head == 0)
	return 0;

    node = list->head;

    list->head->prev = 0;
    list->head = list->head->next;

    if(list->tail == node)
	list->tail = 0;

    node->prev = 0;
    node->next = 0;
    --(list->size);
    return node;
}

void  List_PushBack(List *list, Node *node)
{
    node->prev = list->tail;
    node->next = 0;
    if(list->tail)
	list->tail->next = node;
    list->tail = node;

    if(list->head == 0)
	list->head = node;
    ++(list->size);
}

Node *List_PopBack(List *list)
{
    Node *node = list->tail;
    list->tail = list->tail->prev;
    if(list->tail)
	list->tail->next = 0;

    if(list->head == node)
	list->head = 0;
    node->prev = 0;
    node->next = 0;
    --(list->size);
    return node;
}

void  List_MoveNodeBeforeNode(List *list, Node *fromNode, Node *toNode)
{
    //Get fromNode out of list
    if(fromNode->prev)
	fromNode->prev->next = fromNode->next;

    if(fromNode->next)
	fromNode->next->prev = fromNode->prev;

    //Insert before the toNode
    fromNode->prev = toNode->prev;
    fromNode->next = toNode;
    if(toNode->prev)
	toNode->prev->next = fromNode;
    toNode->prev = fromNode;
    
    //Fix the list's head and tail.
    if(toNode == list->head)
	list->head = fromNode;
    if(fromNode == list->tail)
	list->tail = toNode;
}

#if 0

void  List_MoveNodeAfterNode(List *list, Node *fromNode, Node *toNode)
{
    //Get fromNode out of list
    if(fromNode->prev)
	fromNode->prev->next = fromNode->next;

    if(fromNode->next)
	fromNode->next->prev = fromNode->prev;

    //Insert after the toNode
    fromNode->prev = toNode;
    fromNode->next = toNode->next;
    if(toNode->next)
	toNode->next->prev = fromNode;
    toNode->next = fromNode;
    
    //Fix the list's head and tail.
    if(fromNode == list->head)
	list->head = toNode;
    if(toNode == list->tail)
	list->tail = fromNode;
}

void  List_MoveNodeToHead(List *list, Node *node)
{
    List_MoveNodeBeforeNode(list, node, list->head);
}

#endif // #if 0: Unused Methods

void  List_MoveSubListToHead(List *list, SubList *subList)
{
    if(subList->firstNode == list->head)
	return ;

    //Fix the head and tail
    if(list->tail == subList->lastNode)
	list->tail = subList->firstNode->prev;

    //Get sublist outof the list
    if(subList->firstNode->prev)
	subList->firstNode->prev->next = subList->lastNode->next;

    if(subList->lastNode->next)
	subList->lastNode->next->prev = subList->firstNode->prev;

    //Insert sublist before head
    subList->lastNode->next = list->head;
    if(list->head) {
	list->head->prev = subList->lastNode;
	list->head = subList->firstNode;
	list->head->prev = 0;
    }
}

void  List_AppendNodeToSubList(List *list, Node *node, SubList *subList)
{
    if(subList->firstNode == 0) {
	subList->firstNode = subList->lastNode = node;
	++(subList->size);
	return ;
    }
	
    if(subList->lastNode->next != node)
	List_MoveNodeBeforeNode(list, node, subList->lastNode);
    subList->lastNode = node;
    ++(subList->size);
}

void  List_SortByData(List *list, void *data, SubList *subList)
{
    Node *tmpNode = 0;
    if(list->NodeDataCmp == 0)
	return ;

    tmpNode = list->head;
    SubList_Init(subList);
    while(tmpNode != 0) {
	if((*list->NodeDataCmp)(data, tmpNode->data) == 0) {
	    List_AppendNodeToSubList(list, tmpNode, subList);
	}
	tmpNode = tmpNode->next;
    }
}

#if 0
//Create a new subList
SubList *SubList_New()
{
    SubList *subList = (SubList *) malloc(sizeof(SubList));
    SubList_Init(subList);
    return subList;
}

//Destroy the sublist
void  SubList_Destroy(SubList *subList)
{
    free(subList);
}

#endif //#if 0: Unused Method

void  SubList_Init(SubList *subList)
{
    subList->firstNode = 0;
    subList->lastNode  = 0;
    subList->size      = 0;
}

/***^^^^^^list 的实现^^^^^^***/

/*
 * 这些局部函数是从r_keygen.c中提取出来的，rsa_prime_pool.c中会用到它们。
 *
 */


static unsigned short SMALL_PRIMES[] = { 3, 5, 7, 11, 13, 17, 19, 23 };
static short GenerateDigits (NN_DIGIT *, unsigned short);

static short RSAPrime (NN_DIGIT *, unsigned short, NN_DIGIT *, unsigned short);
static short ProbablePrime (NN_DIGIT *, unsigned short);
static short SmallFactor (NN_DIGIT *, unsigned short);
static short FermatTest (NN_DIGIT *, unsigned short);
static short RelativelyPrime(NN_DIGIT *, unsigned short, 
			     NN_DIGIT *, unsigned short);
static void  FindRSAPrime(NN_DIGIT *a, unsigned short b, NN_DIGIT *c, 
			  unsigned short cDigits, NN_DIGIT *d, unsigned short dDigits);

/* Find a probable prime a between 3*2^(b-2) and 2^b-1, starting at
   3*2^(b-2) + (c mod 2^(b-2)), such that gcd (a-1, d) = 1.

   Lengths: a[cDigits], c[cDigits], d[dDigits].
   Assumes b > 2, b < cDigits * NN_DIGIT_BITS, d is odd,
   cDigits < MAX_NN_DIGITS, dDigits < MAX_NN_DIGITS, and a
   probable prime can be found.
*/
void FindRSAPrime(NN_DIGIT *a, unsigned short b, NN_DIGIT *c, 
		  unsigned short cDigits, NN_DIGIT *d, unsigned short dDigits)
{
    NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS],
	v[MAX_NN_DIGITS], w[MAX_NN_DIGITS];
  
    /* Compute t = 2^(b-2), u = 3*2^(b-2).
     */
    NN_Assign2Exp (t, (unsigned short)(b-2), cDigits);
    NN_Assign2Exp (u, (unsigned short)(b-1), cDigits);
    NN_Add (u, u, t, cDigits);
  
    /* Compute v = 3*2^(b-2) + (c mod 2^(b-2)); add one if even.
     */
    NN_Mod (v, c, cDigits, t, cDigits);
    NN_Add (v, v, u, cDigits);
    if (NN_EVEN (v, cDigits)) {
	NN_ASSIGN_DIGIT (w, 1, cDigits);
	NN_Add (v, v, w, cDigits);
    }
  
    /* Compute w = 2, u = 2^b - 2.
     */
    NN_ASSIGN_DIGIT (w, 2, cDigits);
    NN_Sub (u, u, w, cDigits);
    NN_Add (u, u, t, cDigits);

    /* Search to 2^b-1 from starting point, then from 3*2^(b-2)+1.
     */
    while (! RSAPrime (v, cDigits, d, dDigits)) {
	if (NN_Cmp (v, u, cDigits) > 0)
	    NN_Sub (v, v, t, cDigits);
	NN_Add (v, v, w, cDigits);
    }
  
    NN_Assign (a, v, cDigits);
  
    /* Zeroize sensitive information.
     */
    memset ((POINTER)v, 0, sizeof (v));
}

/* Returns nonzero iff a is a probable prime and GCD (a-1, b) = 1.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits < MAX_NN_DIGITS, bDigits < MAX_NN_DIGITS.
*/
static short RSAPrime(NN_DIGIT *a, unsigned short aDigits, NN_DIGIT *b, unsigned short bDigits)
{
    short status;
    NN_DIGIT aMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS];
  
    NN_ASSIGN_DIGIT (t, 1, aDigits);
    NN_Sub (aMinus1, a, t, aDigits);
  
    status = ProbablePrime (a, aDigits) &&
	RelativelyPrime (aMinus1, aDigits, b, bDigits);

    /* Zeroize sensitive information.
     */
    memset ((POINTER)aMinus1, 0, sizeof (aMinus1));
  
    return (status);
}

/* Returns nonzero iff a is a probable prime.

   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
*/
static short ProbablePrime(NN_DIGIT *a, unsigned short aDigits)
{
    return (! SmallFactor (a, aDigits) && FermatTest (a, aDigits));
}

/* Returns nonzero iff a has a prime factor in SMALL_PRIMES.

   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
*/
static short SmallFactor(NN_DIGIT *a, unsigned short aDigits)
{
    short status;
    NN_DIGIT t[1];
    unsigned short i;
  
    status = 0;
  
    for (i = 0; i < sizeof(SMALL_PRIMES)/sizeof(SMALL_PRIMES)[0]; i++) {
	NN_ASSIGN_DIGIT (t, SMALL_PRIMES[i], 1);
	NN_Mod (t, a, aDigits, t, 1);
	if (NN_Zero (t, 1)) {
	    status = 1;
	    break;
	}
    }
  
    /* Zeroize sensitive information.
     */
    i = 0;
    memset ((POINTER)t, 0, sizeof (t));

    return (status);
}

/* Returns nonzero iff a passes Fermat's test for witness 2.
   (All primes pass the test, and nearly all composites fail.)
     
   Lengths: a[aDigits].
   Assumes aDigits < MAX_NN_DIGITS.
*/
static short FermatTest(NN_DIGIT *a, unsigned short aDigits)
{
    short status;
    NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS];
  
    NN_ASSIGN_DIGIT (t, 2, aDigits);
    NN_ModExp (u, t, a, aDigits, a, aDigits);
  
    status = NN_EQUAL (t, u, aDigits);
  
    /* Zeroize sensitive information.
     */
    memset ((POINTER)u, 0, sizeof (u));
  
    return (status);
}

/* Returns nonzero iff a and b are relatively prime.

   Lengths: a[aDigits], b[bDigits].
   Assumes aDigits >= bDigits, aDigits < MAX_NN_DIGITS.
*/
static short RelativelyPrime (NN_DIGIT *a, unsigned short aDigits, 
			      NN_DIGIT *b, unsigned short bDigits)
{
    short status;
    NN_DIGIT t[MAX_NN_DIGITS], u[MAX_NN_DIGITS];
  
    NN_AssignZero (t, aDigits);
    NN_Assign (t, b, bDigits);
    NN_Gcd (t, a, t, aDigits);
    NN_ASSIGN_DIGIT (u, 1, aDigits);

    status = NN_EQUAL (t, u, aDigits);
  
    /* Zeroize sensitive information.
     */
    memset ((POINTER)t, 0, sizeof (t));
  
    return (status);
}

static short GenerateDigits(NN_DIGIT *a, unsigned short digits)
{
    unsigned short status;
    unsigned char t[MAX_RSA_MODULUS_LEN];

    memset ((POINTER)t, 0, sizeof (t));
    for (status = 0; status < digits * NN_DIGIT_LEN;)
	t[status++] = (unsigned char)(rand()%255 + 1);
    NN_Decode (a, digits, t, (unsigned short)(digits * NN_DIGIT_LEN));

    /* Zeroize sensitive information.
     */
    memset ((POINTER)t, 0, sizeof (t));

    return (0);
}

/***^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^***/


PrimePairPool *PrimePool_New()
{
    int i = 0;
    int j = 0;
    PrimePairPool *primePairPool = 
	(PrimePairPool *)malloc(sizeof(PrimePairPool));

    if(primePairPool == 0)
	return primePairPool;

    memset(primePairPool, 0, sizeof(PrimePairPool));

    primePairPool->r_staticList     = List_New();
    primePairPool->staticList   = List_New();

    primePairPool->r_dynamicList    = List_New();
    primePairPool->dynamicList      = List_New();

    primePairPool->r_staticList->NodeDataCmp   = PrimePair_Cmp;
    primePairPool->staticList->NodeDataCmp     = PrimePair_Cmp;
    primePairPool->r_dynamicList->NodeDataCmp  = PrimePair_Cmp;
    primePairPool->dynamicList->NodeDataCmp    = PrimePair_Cmp;

    primePairPool->dynamicCapacity = DEF_CAPACITY;

#ifdef UNIX
    if(pthread_mutex_init(&primePairPool->mutex,NULL) != 0)
#else  //Windows
	primePairPool->mutex = CreateMutex(NULL,0,"PrimePool_ThreadFunc");
    if(primePairPool->mutex == NULL)
#endif //UNIX
    {
	free(primePairPool);
	primePairPool = 0;
	return primePairPool;
    }

    for(i = 128; i <= 248; i += 16) {
	List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E1,i)));
	List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E2,i)));
    }

    for(i = 128; i <= 248; i += 4) {
	if(i % 16 == 0) {
	    continue;
	}else {
	    List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E1,i)));
	    List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E2,i)));
	}
    }

    for(i = 128; i <= 248; i += 4) {
	if(i % 16 == 0) {
	    for(j = DEF_MORE_NUM; j > 0; --j) {
		List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E1,i)));
		List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E2,i)));
	    }
	}else {
	    for(j = DEF_LESS_NUM; j > 0; --j) {
		List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E1,i)));
		List_PushBack(primePairPool->r_staticList, Node_New(PrimePair_New(DEF_E2,i)));
	    }
	}
    }
    {
	time_t CurTime;
	srand(time(&CurTime));
    }
    return primePairPool;
}

void PrimePool_Start(PrimePairPool *primePairPool)
{
    int ret = 0;
    if((int)primePairPool->tid == 0) {
#ifdef UNIX
	pthread_create(&primePairPool->tid, NULL, PrimePool_ThreadFunc, (void *)primePairPool);

#else //Windows
	primePairPool->handle = CreateThread(NULL, 0, PrimePool_ThreadFunc,
					     primePairPool, 0, &primePairPool->tid);
	ret = SetThreadPriority(primePairPool->handle,THREAD_PRIORITY_LOWEST);
	printf("ret = %d",ret); fflush(stdout);
#endif //UNIX
    }
}

static void PrimePairPool_Lock(PrimePairPool *primePairPool)
{
#ifdef UNIX
    pthread_mutex_lock(&primePairPool->mutex);
#else  //Window
    WaitForSingleObject(primePairPool->mutex,INFINITE); 
#endif //UNIX
}

static void PrimePairPool_Unlock(PrimePairPool *primePairPool)
{
#ifdef UNIX
    pthread_mutex_unlock(&primePairPool->mutex);
#else  //Window
    ReleaseMutex(primePairPool->mutex);  
#endif //UNIX
}


//Request num primePairPools, return the num of the request item.
static void PrimePool_Request(PrimePairPool *primePairPool,
			      long int rsaKeyE, unsigned int rsaKeyLen, int num)
{
    PrimePair     *primePair = 0;
    Node *tmpNode = 0;
    int i = 0;

    if(num <= 0)
	return ;

#ifdef DEBUG
    printf("\n---REQUEST: %d---\n", num); fflush(stdout);
#endif //DEBUG

    if((unsigned int)num > primePairPool->dynamicCapacity / 2) {
	num = primePairPool->dynamicCapacity / 2;
    }
    

    for(i = 0; i < num; ++i) {
	if((primePairPool->r_dynamicList->size + primePairPool->dynamicList->size)
	   <= primePairPool->dynamicCapacity) {
	    List_PushFront(primePairPool->r_dynamicList,
			   Node_New(PrimePair_New(rsaKeyE, rsaKeyLen)));
	} else {
	    if(primePairPool->r_dynamicList->size > primePairPool->dynamicList->size) {
		tmpNode = List_PopBack(primePairPool->r_dynamicList);
		if(tmpNode == 0) {
		    perror("error when pop r_dynamicList");
		    _exit(1);
		}
		PrimePair_Reste((PrimePair *)tmpNode->data, rsaKeyE, rsaKeyLen);
		List_PushFront(primePairPool->r_dynamicList, tmpNode);
	    }else {
		tmpNode = List_PopBack(primePairPool->dynamicList);
		if(tmpNode == 0) {
		    perror("error when pop dynamicList");
		    _exit(1);
		}
		PrimePair_Reste((PrimePair *)tmpNode->data, rsaKeyE, rsaKeyLen);
		List_PushFront(primePairPool->r_dynamicList, tmpNode);
	    }
	}
    }
}

int PrimePool_GetKey(PrimePairPool *primePairPool,
		     unsigned int rsaKeyLen, long int rsaKeyE,
		     R_RSA_PUBLIC_KEY *pPublicKey, 
		     R_RSA_PRIVATE_KEY *pPrivateKey)
{
    SubList   subList;
    PrimePair demo;
    int iRet = 0;
    SubList_Init(&subList);
    PrimePair_Init(&demo);

    if(rsaKeyLen < 128 || rsaKeyLen > 248 || rsaKeyLen % 2) {
	return 1;
    }

    if(rsaKeyE != 3 && rsaKeyE != 65537L) {
	return 1;
    }

    PrimePairPool_Lock(primePairPool);

    demo.rsaKeyE   = rsaKeyE;
    demo.rsaKeyLen = rsaKeyLen;
    
    List_SortByData(primePairPool->dynamicList, &demo, &subList);
    if(subList.size > 0) { //Found in dynamicList
	memcpy(&demo, subList.firstNode->data, sizeof(PrimePair));
	List_MoveSubListToHead(primePairPool->dynamicList, &subList);
	List_PushFront(primePairPool->r_dynamicList,
		       List_PopFront(primePairPool->dynamicList));
	//iRet here used as an tmp var.
	iRet = subList.size;
	List_SortByData(primePairPool->r_dynamicList, &demo, &subList);
	iRet = DEF_MORE_NUM - subList.size - iRet; //One has been moved to r_d list.
	if(iRet > 1 && iRet <= DEF_MORE_NUM) {
	    PrimePool_Request(primePairPool, rsaKeyE, rsaKeyLen, iRet);
	}
    }else {
	List_SortByData(primePairPool->staticList, &demo, &subList);
	if(subList.size > 0) { //Not found in staticList, then create a new one.
	    memcpy(&demo, subList.firstNode->data, sizeof(PrimePair));
	    List_MoveSubListToHead(primePairPool->staticList, &subList);
	    List_PushFront(primePairPool->r_staticList,
			   List_PopFront(primePairPool->staticList));
	}else { //Not found at all
#ifdef DEBUG
	    printf("Nof Found(e=%ld,keyLen=%d)\n",rsaKeyE,rsaKeyLen);fflush(stdout);
#endif //DEBUG
	    PrimePair_Update(&demo, rsaKeyE, rsaKeyLen);
	}
	List_SortByData(primePairPool->r_dynamicList, &demo, &subList);
	iRet = DEF_MORE_NUM - subList.size;
	if(iRet > 1 && iRet <= DEF_MORE_NUM) {
	    PrimePool_Request(primePairPool, rsaKeyE, rsaKeyLen, iRet);
	}
    }

    iRet = 0;
    PrimePairPool_Unlock(primePairPool);
    iRet = PrimePair_GeneratePEMKeys(&demo, pPublicKey, pPrivateKey);
    if(iRet)
	return 1;
    return 0;
}

#ifdef UNIX
static void *PrimePool_ThreadFunc(void *arg)
{
    PrimePairPool *primePairPool = (PrimePairPool *)arg;
    time_t CurTime;

    while(1) {
	if(primePairPool->r_staticList->size == 0 && primePairPool->r_dynamicList->size == 0)
	{
	    perror("sleep PrimePool_ThreadFunc(void *arg)");
	    // 如果缓冲池满，重置随机数并等待5秒钟
	    srand(time(&CurTime));

	    usleep(5*1000000L);

	}else {
	    PrimePairPool_ProcessRequest(primePairPool);
	}
    }
    return 0;
}
#else  //Windows
static DWORD WINAPI PrimePool_ThreadFunc(LPVOID arg)
{
    PrimePairPool *primePairPool = (PrimePairPool *)arg;
    time_t CurTime;

    while(1) {
	if(primePairPool->r_staticList->size == 0 && primePairPool->r_dynamicList->size == 0) {
	    perror("sleep PrimePool_ThreadFunc(void *arg)");
	    // 如果缓冲池满，重置随机数并等待5秒钟
	    srand(time(&CurTime));
	    Sleep(5*1000);    //Sleep 5 seconds
	}else {
	    PrimePairPool_ProcessRequest(primePairPool);
	}
    }
    return 0;
}
#endif //UNIX

static void PrimePairPool_ProcessRequest(PrimePairPool *primePairPool)
{
    Node *dNode = 0;
    Node *sNode = 0;

    PrimePairPool_Lock(primePairPool);    

    if(primePairPool->r_dynamicList->size > 0) {
	dNode = List_PopFront(primePairPool->r_dynamicList);
    }else if(primePairPool->r_staticList->size > 0) {
	sNode = List_PopFront(primePairPool->r_staticList);;
    }else {
	;
    }
    PrimePairPool_Unlock(primePairPool);

    if(dNode) {
	PrimePair_GenPrimes((PrimePair *)dNode->data);
    }

    if(sNode) {
	PrimePair_GenPrimes((PrimePair *)sNode->data);
    }
    
    PrimePairPool_Lock(primePairPool);
    if(dNode)
	List_PushFront(primePairPool->dynamicList, dNode);
    if(sNode)
	List_PushFront(primePairPool->staticList, sNode);
    PRINTT_SIZE(primePairPool);
    PrimePairPool_Unlock(primePairPool);
}

static PrimePair * PrimePair_New(long int rsaKeyE, unsigned int rsaKeyLen)
{
    PrimePair *rsaPrimePair = (PrimePair *)malloc(sizeof(PrimePair));    
    if(rsaPrimePair != 0) {
	rsaPrimePair->rsaKeyE   = rsaKeyE;
	rsaPrimePair->rsaKeyLen = rsaKeyLen;
    }
    return rsaPrimePair;
}

static void PrimePair_Init(PrimePair *primePair)
{
    memset (primePair, 0, sizeof(PrimePair));
}

static int PrimePair_Cmp(void *d1, void *d2)
{
    return memcmp(d1, d2, sizeof(NN_DIGIT) + sizeof(unsigned long));
}

static void PrimePair_Reste(PrimePair *rsaPrimePair, long int rsaKeyE, unsigned int rsaKeyLen)
{
    if(rsaPrimePair != 0) {
	rsaPrimePair->rsaKeyE   = rsaKeyE;
	rsaPrimePair->rsaKeyLen = rsaKeyLen;
    }
}

static void PrimePair_Update(PrimePair *rsaPrimePair, long int rsaKeyE, unsigned int rsaKeyLen)
{
    if(rsaPrimePair != 0) {
	rsaPrimePair->rsaKeyE   = rsaKeyE;
	rsaPrimePair->rsaKeyLen = rsaKeyLen;
    }
    PrimePair_GenPrimes(rsaPrimePair);
}

static void PrimePair_GenPrimes(PrimePair *rsaPrimePair)
{
    NN_DIGIT e[MAX_NN_DIGITS], t[MAX_NN_DIGITS];

    unsigned short nDigits, pDigits;
    unsigned int keyBits = rsaPrimePair->rsaKeyLen * 8;

    nDigits = (keyBits + NN_DIGIT_BITS - 1) / NN_DIGIT_BITS;
    pDigits = (nDigits + 1) / 2;

    GenerateDigits (rsaPrimePair->p, pDigits);
    GenerateDigits (rsaPrimePair->q, pDigits);

    /* NOTE: for 65537, this assumes NN_DIGIT is at least 17 bits. */
    NN_ASSIGN_DIGIT
	(e, rsaPrimePair->rsaKeyE, nDigits);
    FindRSAPrime (rsaPrimePair->p, (unsigned short)((keyBits + 1) / 2), rsaPrimePair->p, pDigits, e, 1);
    FindRSAPrime (rsaPrimePair->q, (unsigned short)(keyBits / 2), rsaPrimePair->q, pDigits, e, 1);

    //***  Sort so that p > q. (p = q case is extremely unlikely.)***/
    //***  If pqCmdFlag < 0, then p < q.  ***//
    /* Sort so that p > q. (p = q case is extremely unlikely.)
     */
    if (NN_Cmp (rsaPrimePair->p, rsaPrimePair->q, pDigits) < 0) {
	NN_Assign (t, rsaPrimePair->p, pDigits);
	NN_Assign (rsaPrimePair->p, rsaPrimePair->q, pDigits);
	NN_Assign (rsaPrimePair->q, t, pDigits);
    }

#ifdef DEBUG
    printf("GenPrimes(e=%ld, l=%ld)%s",
	   rsaPrimePair->rsaKeyE,rsaPrimePair->rsaKeyLen,
	   rsaPrimePair->rsaKeyE == 3 ? "\t\t" : "\t");
    fflush(stdout);
#endif //DEBUG
}

/* Generates an RSA key pair with a given length and public exponent.
 */
// R_RSA_PUBLIC_KEY *publicKey;         /* new RSA public key */
// R_RSA_PRIVATE_KEY *privateKey;       /* new RSA private key */
// R_RSA_PROTO_KEY *protoKey;           /* RSA prototype key */
static 
short PrimePair_GeneratePEMKeys(PrimePair *rsaPrimePair,
				R_RSA_PUBLIC_KEY *publicKey,
				R_RSA_PRIVATE_KEY *privateKey)
{
    NN_DIGIT d[MAX_NN_DIGITS], dP[MAX_NN_DIGITS], dQ[MAX_NN_DIGITS],
	e[MAX_NN_DIGITS], n[MAX_NN_DIGITS], phiN[MAX_NN_DIGITS],
	pMinus1[MAX_NN_DIGITS], qInv[MAX_NN_DIGITS],
	qMinus1[MAX_NN_DIGITS], t[MAX_NN_DIGITS];

    unsigned short nDigits, pDigits;
    unsigned int keyBits = rsaPrimePair->rsaKeyLen * 8;

    if ((keyBits < MIN_RSA_MODULUS_BITS) ||
	(keyBits > MAX_RSA_MODULUS_BITS))
	return (RE_MODULUS_LEN);

    nDigits = (keyBits + NN_DIGIT_BITS - 1) / NN_DIGIT_BITS;
    pDigits = (nDigits + 1) / 2;

    /* NOTE: for 65537, this assumes NN_DIGIT is at least 17 bits. */
    NN_ASSIGN_DIGIT
	(e, rsaPrimePair->rsaKeyE, nDigits);

    /* Compute n = pq, qInv = q^{-1} mod p, d = e^{-1} mod (p-1)(q-1),
       dP = d mod p-1, dQ = d mod q-1.
    */
    NN_Mult (n, rsaPrimePair->p, rsaPrimePair->q, pDigits);
    NN_ModInv (qInv, rsaPrimePair->q, rsaPrimePair->p, pDigits);
  
    NN_ASSIGN_DIGIT (t, 1, pDigits);
    NN_Sub (pMinus1, rsaPrimePair->p, t, pDigits);
    NN_Sub (qMinus1, rsaPrimePair->q, t, pDigits);
    NN_Mult (phiN, pMinus1, qMinus1, pDigits);

    NN_ModInv (d, e, phiN, nDigits);
    NN_Mod (dP, d, nDigits, pMinus1, pDigits);
    NN_Mod (dQ, d, nDigits, qMinus1, pDigits);
  
    publicKey->bits = privateKey->bits = keyBits;
    NN_Encode (publicKey->modulus, MAX_RSA_MODULUS_LEN, n, nDigits);
    NN_Encode (publicKey->exponent, MAX_RSA_MODULUS_LEN, e, 1);
    memcpy
	((POINTER)privateKey->modulus, (POINTER)publicKey->modulus,
	 MAX_RSA_MODULUS_LEN);
    memcpy
	((POINTER)privateKey->publicExponent, (POINTER)publicKey->exponent,
	 MAX_RSA_MODULUS_LEN);
    NN_Encode (privateKey->exponent, MAX_RSA_MODULUS_LEN, d, nDigits);
    NN_Encode (privateKey->prime[0], MAX_RSA_PRIME_LEN, rsaPrimePair->p, pDigits);
    NN_Encode (privateKey->prime[1], MAX_RSA_PRIME_LEN, rsaPrimePair->q, pDigits);
    NN_Encode (privateKey->primeExponent[0], MAX_RSA_PRIME_LEN, dP, pDigits);
    NN_Encode (privateKey->primeExponent[1], MAX_RSA_PRIME_LEN, dQ, pDigits);
    NN_Encode (privateKey->coefficient, MAX_RSA_PRIME_LEN, qInv, pDigits);
   
    /* Zeroize sensitive information.
     */
    memset ((POINTER)d, 0, sizeof (d));
    memset ((POINTER)dP, 0, sizeof (dP));
    memset ((POINTER)dQ, 0, sizeof (dQ));
    //memset ((POINTER)rsaPrimePair->p, 0, MAX_NN_DIGITS);
    memset ((POINTER)phiN, 0, sizeof (phiN));
    memset ((POINTER)pMinus1, 0, sizeof (pMinus1));
    //memset ((POINTER)rsaPrimePair->q, 0, MAX_NN_DIGITS);
    memset ((POINTER)qInv, 0, sizeof (qInv));
    memset ((POINTER)qMinus1, 0, sizeof (qMinus1));
    memset ((POINTER)t, 0, sizeof (t));
#ifdef DEBUG
    printf("\tGenKey(%ld, %ld)>>\n",
	   rsaPrimePair->rsaKeyE,rsaPrimePair->rsaKeyLen);
    fflush(stdout);
#endif //DEBUG
    return (0);
}

