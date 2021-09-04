# Project2 - disk-based b+tree (Milestone 1)
## 목차
1. Possible call path of the insert/delete operation
   1. insert
   2. delete
   3. find
   4. etc
2. Detail flow of the structure modification (split, merge)
   1. insert (split)
   2. delete (coalesce, redistribute)
3. (Naive) designs or required changes for building on-disk b+ tree
   1. page
   2. file manager API
   3. open, insert, find, delete
   

------

## 1. Possible call path of the insert/delete operation

### 1-1. insert
```
**처음 tree가 만들어지는 경우**
main -> insert -> make_record -> start_new_tree

**만들어진 tree에 insert하는데 overflow가 나지 않는 경우**
main -> insert -> make_record -> find_leaf -> insert_into_leaf

**만들어진 tree에 insert하는데 overflow가 나는 경우**
main -> insert -> make_record -> find_leaf -> insert_into_leaf_after_splitting


***find_leaf은 1-3에서 insert_into_leaf_after_splitting 2-1에서 구체적으로 설명하겠습니다***
``` 
<br/><br/>

**insertion**의 시작은 **main.c**의 **main**함수에서 46번째줄의 **scanf**함수에서 입력값으로 i를 받았을 때부터 입니다. 54번째 줄에서 key 값을 입력받고 **insert** 함수가 call됨에 따라 본격적으로 **insertion**이 시작됩니다.
<br/><br/>

이후 **bpt.c**의 insert 함수를 살펴보면 그 원형은
```c
node * insert( node * root, int key, int value )
```
인데 이는 root노드, key값, value 값을 인자로 받아 insertion을 실행 한 뒤 root노드를 return 하겠다는 뜻이 됩니다.


이후 **insert** 함수 내부를 살펴보면 첫번째로 812번째줄에서
```c
if (find(root, key, false) != NULL)
  return root;
```
코드를 확인할 수 있는데 이는 tree에 이미 있는 값이라면 더이상 insert를 진행하지 않고 root를 반환한다는 것을 의미합니다. 이 **find**함수는 1-3에서 자세히 기술하겠습니다.


818번째줄에서
```c
pointer = make_record(value);
```
를 확인할 수 있는데 이는 pointer라는 이름의 변수에 **make_record**함수를 호출하여 record를 1개 만들겠다는 뜻입니다. record란 leaf node에서 key에 대응되는 value를 저장하는 구조체인데 이 구조체와 **make_record**함수는 1-4에서 자세히 기술하겠습니다.


825번째줄에서
```c
if (root == NULL) 
  return start_new_tree(key, pointer);
```
를 확인할 수 있는데 이는 root가 NULL. 즉 처음 tree를 시작할때의 경우를 나타낸다고 볼 수 있겠습니다.


833번째줄에서
```c
leaf = find_leaf(root, key, false);
```
를 확인할 수 있는데 이는 현재 tree에서 key값이 들어갈 node를 찾아주는 과정입니다.
**find_leaf**함수는 해당 key가 있는 node나 해당 key가 들어갈 node를 찾아주는 함수인데 구체적인 과정은 **find**함수를 설명할때 같이 하겠습니다.


838번째줄에서
```c
if (leaf->num_keys < order - 1) {
  leaf = insert_into_leaf(leaf, key, pointer);
  return root;
}
```
를 볼 수 있는데 이는 현재 key가 들어갈 node에 새로운 node를 넣어도 overflow가 발생하지 않아 split 할 필요가 없는 경우입니다.

이후 **insert_into_leaf**함수에 대해 살펴보겠습니다.
```c
node * insert_into_leaf( node * leaf, int key, record * pointer ) {

    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
        insertion_point++;

    for (i = leaf->num_keys; i > insertion_point; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    leaf->keys[insertion_point] = key;
    leaf->pointers[insertion_point] = pointer;
    leaf->num_keys++;
    return leaf;
}
```
첫번째 while문에서 key가 들어갈 곳을 찾고 그다음 for문에서 key가 들어갈 자리를 만들어주기 위해 기존 값들을 한칸씩 뒤로 밀어줍니다. 이후 해당위치에 값들을 넣어주고 return 합니다.


847번째줄에서
```c
return insert_into_leaf_after_splitting(root, leaf, key, pointer);
```
를 볼 수 있는데 이는 현재 key가 들어갈 node에 새로운 node를 넣으면 overflow가 발생하여 insertion 후 split을 통한 적절한 조치가 필요한 상황입니다.
이후 **insert_into_leaf_after_splitting**함수는 2. Detail flow of the structure modification (split, merge)에서 살펴보겠습니다.

### 1-2. delete
```
**해당 키가 없거나 해당 키가 속한 노드를 찾지 못했을 때**
main -> delete -> find -> find_leaf

**해당 키가 있고 해당 키가 속한 노드를 찾았을 때**
  ***만약 coalesce와 redistribute이 필요하더라도 delete_entry함수가 안에서 다른 함수를 호출하니까 처리 가능***
main -> delete -> find -> find_leaf -> delete_entry

***find와 find_leaf은 1-3에서 ***
```
<br/><br/>

**deletion**의 시작은 **main.c**의 **main**함수에서 46번째줄의 **scanf**함수에서 입력값으로 d를 받았을 때부터 입니다. 48번째 줄에서 key 값을 입력받고 **delete** 함수가 call됨에 따라 본격적으로 **deletion**이 시작됩니다.


이후 **bpt.c**의 delete 함수를 살펴보면 그 원형은
```c
node * delete(node * root, int key)
```
인데 이는 root노드, key값을 인자로 받아 deletion을 실행 한 뒤 root노드를 return 하겠다는 뜻이 됩니다.


이후 **delete** 함수 내부를 살펴보면
```c
node * key_leaf;
record * key_record;

key_record = find(root, key, false);
key_leaf = find_leaf(root, key, false);
if (key_record != NULL && key_leaf != NULL) {
  root = delete_entry(root, key_leaf, key, key_record);
  free(key_record);
}
return root;
```
코드를 확인할 수 있는데 이는 tree에 없는 값이거나 해당 값을 찾는 leaf node를 찾지 못한다면 더이상 delete를 진행하지 않고 return한다는 것을 볼 수 있습니다.. 이 **find**함수와 **find_leaf**함수는 1-3에서 자세히 기술하겠습니다. 그리고 **delete_entry**함수는 2. Detail flow of the structure modification (split, merge)에서 살펴보겠습니다.


### 1-3. find

**find**종류의 함수는 **main**함수를 비롯한 여러 함수에서 해당 key값을 찾을때 사용됩니다. 이 절에서는 **find**함수를 비롯한 search기능을 하는 함수들에 대해 기술하겠습니다.


```c
void find_and_print(node * root, int key, bool verbose)
```
해당 키의 record를 찾은 후 print 합니다.<br/><br/>

```c
void find_and_print_range( node * root, int key_start, int key_end,
        bool verbose )
```
key_start와 key_end 사이의 존재하는 key값들과 key값들의 record를 출력합니다. key_start와 key_end 역시 포함합니다.<br/><br/>

```c
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[])
```
key_start와 key_end 사이의 존재하는 값들의 keys와 pointers를 찾고 저장해주는 함수입니다. key_start와 key_end 역시 포함합니다.<br/><br/>

```c
node * find_leaf( node * root, int key, bool verbose )
```
해당 키가 존재하는 leaf node나 해당 키가 들어갈 leaf node를 찾아서 반환해주는 함수입니다.<br/><br/>

```c
record * find( node * root, int key, bool verbose )
```
key가 존재하는 node를 찾고 거기서 해당하는 key의 record를 반환해주는 함수입니다.<br/><br/>

```c
void find_and_print(node * root, int key, bool verbose)
```
해당 키를 찾은 후 print 합니다.


### 1-4. etc

이 절에서는 **insertion**, **deletion** 에 사용되는 여러 부가적인 함수들과 구조체에 대해 살펴보겠습니다.


```c
int cut( int length )
```
length로 들어온 값이 짝수면 /2 해주고 홀수면 /2+1 해줍니다. 보통 우리는 이 함수의 parameter로 order등을 주어 split 될 지점이나 underflow가 일어나는지 검사하게 됩니다. <br/><br/>

```c
record * make_record(int value)
```
record 1개를 만드는 함수입니다. node의 key에 해당하는 value값을 저장합니다.<br/><br/>

```c
node * make_node( void )
```
node 1개를 만드는 함수 입니다. 단순히 빈 node를 만듭니다.<br/><br/>

```c
node * make_leaf( void )
```
leaf node 1개를 만드는 함수입니다.<br/><br/>

```c
int get_left_index(node * parent, node * left)
```
left로 들어온 node가 parent의 몇번째 pointer가 가리키고 있는지 찾아서 return 해줍니다.<br/><br/>

```c
int get_neighbor_index( node * n )
```
자신의 왼쪽 형제 노드의 부모 노드에서의 index를 반환해주는 함수입니다. 가장 왼쪽 노드라면 -1을 반환합니다.<br/><br/>

```c
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;
```
노드에 사용될 node구조체 입니다. 주의해야할 점은 leaf node가 아니라면 valid한 pointer의 수는 num_key+1개 라는 것입니다. leaf node에서는 valid한 pointer의 수가 num_key와 같은데 이는 leaf node의 마지막 포인터는 record를 가리키고 있는 것이 아닌 다음 leaf node를 가리키고 있기 때문입니다.

------

## 2. Detail flow of the structure modification (split, merge)

### 1. insert (split)
```
제공된 코드로 구현된 B+tree는 node에 order-1개까지의 key를 가지고 있을 수 있습니다. 
따라서 1개의 node에 order개 만큼의 key가 들어가게 된다면 해당 node는 split하게 됩니다. 
그 과정을 이 절에서 설명해보겠습니다.
``` 
<br/><br/>

1-1에서 살펴본 바와 같이 insert는 처음 empty 트리에 대해서는 tree를 만들어 주고 그렇지 않다면 트리를 확장해나가게 됩니다. 그과정에서 현제 key를 집어넣는 node에 key를 넣었을 경우 overflow가 나지 않는다면 **insert_into_leaf**함수를 호출해서 넣어주면 되고 overflow가 난다면 **insert_into_leaf_after_splitting**함수를 호출해주어야 합니다. overflow가 나지 않는 경우는 간단하여 1-1에서 모두 살펴보았으니 overflow가 나는 경우를 살펴보겠습니다.<br/><br/>

```c
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer)
```
overflow가 났을경우 가장 먼저 불려지는 함수 입니다. leaf node에서 overflow가 났다고 보고 leaf node를 split 해줍니다. 

```c
insertion_index = 0;
 while (insertion_index < order - 1 && leaf->keys[insertion_index] < key)
     insertion_index++;

 for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
     if (j == insertion_index) j++;
     temp_keys[j] = leaf->keys[i];
     temp_pointers[j] = leaf->pointers[i];
 }

 temp_keys[insertion_index] = key;
 temp_pointers[insertion_index] = pointer;

 leaf->num_keys = 0;

 split = cut(order - 1);

 for (i = 0; i < split; i++) {
     leaf->pointers[i] = temp_pointers[i];
     leaf->keys[i] = temp_keys[i];
     leaf->num_keys++;
 }

 for (i = split, j = 0; i < order; i++, j++) {
     new_leaf->pointers[j] = temp_pointers[i];
     new_leaf->keys[j] = temp_keys[i];
     new_leaf->num_keys++;
 }
```
insertion_index를 찾고 temp_keys와 tmep_pointers에 overflow가 난 node를 모조리 저장해줍니다.(새로운 값을 넣기전의 node가 아닌 새로운 값을 넣어 overflow가 난 node를 말합니다.) split은 말그대로 두개로 나뉘어질 지점을 뜻합니다. cut 함수는 1-4절에서 살펴보았습니다.

이후
```c
new_leaf->pointers[order - 1] = leaf->pointers[order - 1];
 leaf->pointers[order - 1] = new_leaf;

 for (i = leaf->num_keys; i < order - 1; i++)
     leaf->pointers[i] = NULL;
 for (i = new_leaf->num_keys; i < order - 1; i++)
     new_leaf->pointers[i] = NULL;

 new_leaf->parent = leaf->parent;
 new_key = new_leaf->keys[0];
```
두개로 나뉘어진 node의 pointer들을 적절하게 초기화 시켜주고 부모 node까지 초기화 시켜줍니다. 

```c
return insert_into_parent(root, leaf, new_key, new_leaf);
```
모두 마치면 split된 두 node의 index 역할을 해줄 new_key를 부모 노드에 삽입해줍니다.<br/><br/><br/><br/>

```c
node * insert_into_parent(node * root, node * left, int key, node * right) 
```
이제 internal node에 주목할 차례입니다. 아래 나오는 함수들은 각각의 case들을 해결합니다.<br/><br/>

case1
```c
if (parent == NULL)
        return insert_into_new_root(left, key, right); 
```
parent가 null일 경우 **insert_into_new_root**함수로 새로운 root를 만들어주면 끝입니다.<br/><br/>

그외의 경우에는
```c
left_index = get_left_index(parent, left);
```
1-4절에서 설명한대로 left_index를 구해주고

case2
```c
if (parent->num_keys < order - 1)
        return insert_into_node(root, parent, left_index, key, right);
```
parent에서 overflow가 일어나지 않는다면 **insert_into_node**함수로 node에 해당 key를 추가해주기만 하면 됩니다.

```c
node * insert_into_node(node * root, node * n, 
        int left_index, int key, node * right) {
    int i;

    for (i = n->num_keys; i > left_index; i--) {
        n->pointers[i + 1] = n->pointers[i];
        n->keys[i] = n->keys[i - 1];
    }
    n->pointers[left_index + 1] = right;
    n->keys[left_index] = key;
    n->num_keys++;
    return root;
}
```
**insert_into_node**함수입니다. key가 들어갈 자리를 마련해주기 위해 한칸씩 밀어주고 해당 자리에 key를 넣어주고 포인터와 num_keys를 설정해주면 끝입니다.
<br/><br/>

case3
```c
return insert_into_node_after_splitting(root, parent, left_index, key, right);
```
parent에서 overflow가 일어나면 **insert_into_node_after_splitting**함수로 node에 key 값을 넣고 split하면 됩니다.

```c
node * insert_into_node_after_splitting(node * root, node * old_node, int left_index, 
        int key, node * right)
```
**insert_into_node_after_splitting**함수입니다. 내부구조는 우리가 leaf node를 split 할때와 크게 다르지 않습니다. 다만 주의해야 할 점은 1-4에서도 이야기 했듯이 internal node는 leaf node와는 다르게 valid한 pointer의 수가 num_keys+1이라는 사실입니다. 따라서 temp를 초기화 할때나 둘로 나뉜 node를 초기화 할때 이점을 주의해서 초기화 해주어야 합니다.

```c
return insert_into_parent(root, old_node, k_prime, new_node);
```
마지막으로 insert_into_parent를 호출하여 split 된 두 node의 index를 parent에게 올려줍니다. 따라서 insert_into_parent는 split이 일어날때마다 불리게 되고 case1, 2, 3의 경우들을 적절히 수행하며 split이 끝날때 비로서 모든 insertion 작업이 끝나게 됩니다.


### 2. delete (coalesce, redistribute)
```
제공된 코드로 구현된 B+tree는 leaf node는 underflow가 날 수 있습니다.
따라서 node가 root가 아닌 internal일 경우 cut(order-1)개, leaf일 경우 cut(order) -1 개보다 크거나 같은 수의 key를 가지고 있어야 합니다.
그렇지 않다면 coalesce이나 redistribute가 일어나게 됩니다.
만약 underflow가 났을때 neighbor의 key수와 현재 node의 key수의 합이 capacity보다 적다면 coalesce, 크거나 같다면 redistribute가 일어납니다.
(capacity : leaf일때 order, 아닐때 order-1)
그 과정을 이 절에서 설명해보겠습니다.
``` 
<br/><br/>

1-1에서 살펴본 바와 같이 delete는 처음 트리에 대해서 해당 key와 그 key가 존재하는 leaf_node를 찾습니다. 이후 해당 key 값이 존재하고 해당 key 값을 담고 있는 node가 존재한다면 **delete_entry**함수를 호출해서 본격적으로 삭제를 시작합니다.<br/><br/>

```c
node * delete_entry( node * root, node * n, int key, void * pointer {
  //...
  n = remove_entry_from_node(n, key, pointer);
  //...
})
```
**delete_entry**함수가 실행되고 나면 먼저 **remove_entry**함수가 실행되어 해당 키를 삭제해줍니다. underflow가 날 경우 **delete_entry**함수 후반부에서 처리해주므로 일단 삭제합니다.<br/><br/>

```c
node * remove_entry_from_node(node * n, int key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}
```
1개의 entry를 삭제해주는 함수입니다. 해당 위치를 찾고 해당 위치 뒤에서부터 한칸씩 앞으로 당겨줍니다. 역시 주의해야 할점은 leaf와 leaf가 아닌 node의 valid한 pointer수 입니다.<br/><br/>

이후 다시 **delete_entry**함수로 돌아옵니다.
```c
if (n == root) 
        return adjust_root(root);
```
만약 root노드에서 삭제연산이 일어났다면 그에 맞게 root를 조정해주어야 합니다.<br/><br/>

```c
node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}
```
case1
```c
if (root->num_keys > 0)
        return root;
```
root에서 삭제가 일어났지만 key가 존재하는 경우 그대로 return 합니다.<br/><br/>

case2
```c
if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }
```
root에서 삭제가 일어나 key가 존재하지 않게 되었기에 자식 node들 중 가장 왼쪽에 있는 node가 root가 됩니다().<br/><br/>

case3
```c
else
        new_root = NULL;
```
root가 비어있고 leaf 노드라면 empty tree입니다.
<br/><br/>

이후 다시 **delete_entry**함수로 돌아옵니다.
```c
min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;
if (n->num_keys >= min_keys)
       return root;
neighbor_index = get_neighbor_index( n );
 k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
 k_prime = n->parent->keys[k_prime_index];
 neighbor = neighbor_index == -1 ? n->parent->pointers[1] : 
     n->parent->pointers[neighbor_index];

 capacity = n->is_leaf ? order : order - 1;

 if (neighbor->num_keys + n->num_keys < capacity)
     return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

 else
     return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
```
num_keys >= min_key가 되지 못했고 underflow가 났고 이를 해결해줘야 하는 상황입니다. neighbor_index에 1-4절에서 설명한 get_neighbor_index함수를 이용해 index를 구해주고 가장 왼쪽 node이냐 아니냐를 따라서 k_prime_index, k_prime, neighbor에 적절한 값을 초기화 해줍니다. 그리고 2-2절의 시작부분에서 설명한 조건에 따라 coalesce나 redistribution을 진행합니다. 따라서 neighbor node와 coalesce를 할 수 있으면 coalsece를, 그렇지 않다면 neighbor에서 key를 가져와 redistribution을 하게 됩니다. <br/><br/>

case1
```c
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime)
```
coalesc_nodes 함수 입니다.
```c
if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

neighbor_insertion_index = neighbor->num_keys;
```
먼저 가장 왼쪽 node에 underflow가 나서 coalesce 되는 경우에는 이 과정이 필요합니다. 이후 코드에서 neighbor는 모두 왼쪽에 있는 node라고 생각하고 진행되기 때문입니다. ( ex) n=2 neighbor = 1 <현재는 n=1 neighbor =2 이므로  스왑시켜줘야함> )
다음으로는 neighbor_insertion_index에 neighbor->num_keys를 초기화시켜줘 이어붙일 위치를 정해줍니다.
.<br/><br/>

case1-1
```c
if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }
```
internal node에서 coalesce가 일어나는 경우 입니다. 구한 neighbor_insertion_index에 구한 k_prime 값을 넣어주고 이후 n의 key와 pointer들을 neighbor로 옮겨줍니다. 마지막으로 옮긴 pointer들의 parent를 neighbor로 설정해줍니다. .<br/><br/>

case1-2
```c
else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }
```
leaf node에서 coalesce가 일어나는 경우 입니다. 구한 neighbor_insertion_index을 이용해 두 노드를 합쳐주고 마지막으로 neighbor의 마지막 포인터(다음 leaf node를 가리키는)을 설정해줍니다.<br/><br/>

다시 **coalesce_nodes**로 돌아와서
```c
root = delete_entry(root, n->parent, k_prime, n);
```
부모 노드에 대해 delete_entry를 재귀적으로 불러줍니다.<br/><br/><br/><br/>


다시 **delete_entry**로 돌아와서
case2
```c
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, 
        int k_prime_index, int k_prime)
```
redistribute_nodes 함수 입니다. n은 neighbor에서 1개를 받는 노드입니다.

case2-1
```c
if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }
```
먼저 가장 왼쪽 node가 아닌 node에서 underflow가 나서 coalesce 되는 경우입니다. internal인 경우 leaf보다 valid pointer가 1개 더 많으니까 먼저 조치를 취해주고 for 문을 돌며 key와 pointer들을 한칸씩 오른쪽으로 밀어줍니다.<br/><br/>

case2-1-1<br/><br/>
그 뒤 internal인 경우 n의 가장 왼쪽 포인터가 가리키고 있는 곳을 neighbor의 가장 오른쪽이 가리키고 있던 곳으로 설정해주고 (왼쪽에서 오른쪽으로 key가 넘어가는거니까) 이 넘어온 key의 pointer가 가리키고 있던 곳의 parent를 n으로 설정해주고 neighbor의 가장 오른쪽의 포인터가 가리키고 있는걸 NULL로 만들어주고 k prime으로 n의 첫번째 키으로 값 설정합니다. 마지막으로 n의 parent가 neighbor를 가리키고 있던 index의 값을 바꿔줍니다.

case2-1-2<br/><br/>
그 뒤 leaf인 경우 n의 가장 왼쪽 포인터가 가리키는 곳을 neighbor의 가장 오른쪽이 가리키고 있던 record를 가리키게 해주고 neighbor의 가장 오른쪽 포인터가 가리키고 있는 곳을 NULL로 n의 가장 왼쪽 키 값을 neighbor에서 가져온 값으로 설정하고 n의 parent가 leaf를 가리키고 있던 key 값을 가져온 값으로 설정합니다.
.<br/><br/>

case2-2
```c
else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }
```
n이 가장 왼쪽 node인 경우입니다. 먼저 n이 leaf node라면 n의 뒤에 neighbor에서 가져온 값의 정보들을 추가해주면 됩니다. internal일때도 마찬가지로 뒤에 여태까지 구해준 정보들을 추가해주면 됩니다. 이후 for문을 돌며 neighbor의 정보들을 한칸씩 앞으로 당겨주고면 됩니다. 마지막 if문은 internal일 경우 leaf보다 valid pointer의 수가 1개 더 많기 때문에 작성합니다.<br/><br/>

이후 **redistribute_nodes**로 돌아와서 n과 neighbor의 num_keys들을 조정해주고 root를 반환합니다.

------

## 3. (Naive) designs or required changes for building on-disk b+ tree

### 3-1. page
in-memory b+tree를 on-disk b+tree로 바꾸는 과정에서 우리는 page를 사용하게 됩니다. 즉 db파일에 header page, free page, internal page, leaf page 등을 만들어 데이터들을 효과적으로 저장&관리할 것입니다. 

그러기 위해선 on-disk에서 쓰이는 page 저장형태와 in-memory에 쓰이는 page 구조체가 같은게 좋습니다. 따라서 file.h 에 page 구조체를 선언할 것입니다.

 header와 record 등 page 내부에 들어갈 자료를 struct로 선언하고 각 type별 page마다 필요한 요소를 묶어 또 구조체를 만들 것입니다. 그리고 제공된 struct page_t에서 union으로 각 페이지를 묶어 필요한 유형을 선택하여 사용 할 수 있게 만들겠습니다.

 ```c
typedef struct internalPage{
    union
    {
        struct{
            Pheader header;
            leafRecord records[INTERNAL_ORDER];
        };
        char size[PAGE_SIZE];
    };
}internalPage;

 struct page_t {
    union
    {
        headerPage H;
        freePage F;
        internalPage I;
        leafPage L;
    };
};
 ```
 
 ### 3-2. file manager API
 일단 file manager는 **file_alloc_page**, **file_free_page**, **file_read_page**, **file_write_page**로 이루어져있습니다.

 **file_alloc_page**는 free page 1개를 가져오는 함수로서 header page(pagenum_t == 0)의 free page를 관리하는 부분으로 가서 free page를 가저오겠습니다.

 **file_free_page**는 page를 free page로 만든 뒤 free page list에 추가해주는 함수입니다. 값을 모두 초기화 시켜준 뒤 freePage타입을 선택하고 free page list에 연결시켜주겠습니다.

 **file_read_page**는 해당 pagenum의 page로 가서 정보를 읽어온 뒤 dest에 저장해주는 함수입니다. pagenum*PAGE_SIZE로 offset을 구해준 뒤 file read와 file seek 함수등을 이용해 적절한 정보를 dest에 담겠습니다.

 **file_write_page**는 src의 정보를 해당 pagenum의 page로 가서 정보를 저장하는 함수입니다. pagenum*PAGE_SIZE로 offset을 구해준 뒤 file write와 file seek 함수등을 이용해 적절한 정보를 disk에 저장하겠습니다.


### 3-3. open, insert, find, delete
구현해야할 service는 open, insert, find, delete입니다. 이 기능들은 구체적으로 **open_table**, **db_insert**, **db_find**, **db_delete**들의 함수로 구현됩니다.

**open_table**는 database 파일을 여는 함수입니다. 만약 db파일이 없다면 만들어주어야 하는데 db파일의 size는 4096(PAGE_SIZE)*1024(HEADER page + 1023개의 FREE PAGE)로 하겠습니다. unique_id를 return 하게 되어있는데 4096*1024까지의 크기가 table 0으로 보고 0을 return 하겠습니다.

**db_insert**는 record를 넣는 함수입니다. 제공된 코드의 기본적인 원리를 따라가며 동작하게 만들 것입니다. 다만 새로 만든 find 함수를 이용해 위치를 적절히 찾고 memory에 쓰는 부분을 file manager API를 이용해 disk에 작성하게 만들어 줄 것입니다.

**db_find**는 key에 맞는 value를 찾아주는 함수입니다. header page를 읽고 root page를 찾은 뒤 제공된 코드에서의 find함수의 원리에 변경된 자료구조의 형태를 적용시켜 find를 할 생각입니다.

**db_delete**는 key에 해당되는 record를 찾아서 삭제해주는 함수입니다. 새로 만든 find 함수를 이용해 위치를 적절히 찾고 해당 record를 삭제시킬 것입니다. 역시 제공된 코드의 기본적인 원리를 따라가며 동작하게 만들 것인데 delayed merge를 적용시켜 구현할 것입니다. 또한 record를 삭제시키고 page를 조작하는 것 뿐만 아니라 다쓴 페이지는 free page로 만들어 free page list로 관리해주는 일까지 해줄 것입니다.