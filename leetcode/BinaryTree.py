class TreeNode:
    def __init__(self, value=0, left=None, right=None):
        self.value = value
        self.left = left
        self.right = right


def depthFirstTraversal(root):
    """
    DFS Traversal - Preorder (Root -> Left -> Right)
        
    This is the most common DFS traversal. We visit the root first,
    then recursively traverse the left subtree, then the right subtree.
        
    Algorithm:
    1. If the node is None, return (base case)
    2. Process the current node (add to result)
    3. Recursively traverse the left subtree
    4. Recursively traverse the right subtree
        
    Time Complexity: O(n) where n is the number of nodes
    Space Complexity: O(h) where h is the height of the tree
                - O(log n) for balanced trees
                - O(n) for skewed trees (worst case)
        
    Args:
        root: TreeNode - the root of the tree
     Returns:
        list - values in DFS preorder
    """

    """
    Creates a sample tree:
            1
           / \
          2    3
         / \  / \
        4   5 6 7
    """

    out = []
    if root == None:
        return 
        
    print(root.value, end=' ')
    depthFirstTraversal(root.left)
    depthFirstTraversal(root.right)



def dfs_inorder(root):
    """
    DFS Inorder Traversal (Left -> Root -> Right)
    
    For binary search trees, this gives values in sorted order!
    
    Algorithm:
    1. If the node is None, return (base case)
    2. Recursively traverse the left subtree
    3. Process the current node (add to result)
    4. Recursively traverse the right subtree
    
    Time Complexity: O(n)
    Space Complexity: O(h)
    
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in DFS inorder
    """

    """
    Creates a sample tree:
            1
           / \
          2    3
         / \  / \
        4   5 6  7
    """

    if root == None:
        return
    
    dfs_inorder(root.left)
    print(root.value, end=' ')
    dfs_inorder(root.right)


    pass


def dfs_postorder(root):
    """
    DFS Postorder Traversal (Left -> Right -> Root)
    
    Useful for deleting trees or evaluating expressions.
    
    Algorithm:
    1. If the node is None, return (base case)
    2. Recursively traverse the left subtree
    3. Recursively traverse the right subtree
    4. Process the current node (add to result)
    
    Time Complexity: O(n)
    Space Complexity: O(h)
    
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in DFS postorder
    """

    """
            1
           / \
          2    3
         / \  / \
        4   5 6  7
    """

    if root == None:
        return
    
    dfs_postorder(root.left)
    dfs_postorder(root.right)
    print(root.value, end=' ')

    pass


def dfs_iterative_preorder(root):
    """
    Iterative DFS Preorder using an explicit stack.
    
    Sometimes preferred when:
    - Avoiding recursion overhead
    - Deep trees (avoiding stack overflow)
    - Need more control over the traversal
    
    Algorithm:
    1. If root is None, return empty list
    2. Initialize a stack with the root node
    3. While stack is not empty:
       a. Pop a node from the stack
       b. Process the node (add to result)
       c. Push right child first (if exists)
       d. Push left child second (if exists)
       Note: We push right before left because stack is LIFO,
             so left will be popped and processed first
    
    Hint: Remember that stacks are LIFO (Last In First Out)!
    
    Time Complexity: O(n)
    Space Complexity: O(h) - explicit stack
    
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in DFS preorder
    """

    output = []

    stack = [root]

    if root == None:
        return output

    while len(stack) > 0:

        node = stack.pop()

        output.append(node.value)

        if node.right:
            stack.append(node.right)
        if node.left:
            stack.append(node.left)

    print(f"\nThe iterative dfs preorder solution is {output}")    
    pass


def dfs_iterative_inorder(root):
    """
    Iterative DFS Preorder using an explicit stack.
    
    Sometimes preferred when:
    - Avoiding recursion overhead
    - Deep trees (avoiding stack overflow)
    - Need more control over the traversal
    
    Algorithm:
    1. If root is None, return empty list
    2. Initialize a stack with the root node
    3. While stack is not empty:
       a. Pop a node from the stack
       b. Process the node (add to result)
       c. Push right child first (if exists)
       d. Push left child second (if exists)
       Note: We push right before left because stack is LIFO,
             so left will be popped and processed first
    
    Hint: Remember that stacks are LIFO (Last In First Out)!
    
    Time Complexity: O(n)
    Space Complexity: O(h) - explicit stack
    
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in DFS preorder
    """

    """
            1
           / \
          2    3
         / \  / \
        4   5 6  7
    """

    output = []

    stack = [root]

    if root == None:
        return output

    while len(stack) > 0:

        node = stack.pop()

        if node.right:
            stack.append(node.right)

        output.append(node.value)

        if node.left:
            stack.append(node.left)

    print(f"\nThe iterative dfs inorder solution is {output}")    
    pass


def dfs_iterative_postorder(root):
    """
    Iterative DFS Preorder using an explicit stack.
    
    Sometimes preferred when:
    - Avoiding recursion overhead
    - Deep trees (avoiding stack overflow)
    - Need more control over the traversal
    
    Algorithm:
    1. If root is None, return empty list
    2. Initialize a stack with the root node
    3. While stack is not empty:
       a. Pop a node from the stack
       b. Process the node (add to result)
       c. Push right child first (if exists)
       d. Push left child second (if exists)
       Note: We push right before left because stack is LIFO,
             so left will be popped and processed first
    
    Hint: Remember that stacks are LIFO (Last In First Out)!
    
    Time Complexity: O(n)
    Space Complexity: O(h) - explicit stack
    
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in DFS preorder
    """

    output = []

    stack = [root]

    if root == None:
        return output

    while len(stack) > 0:

        node = stack.pop()

        if node.right:
            stack.append(node.right)
        if node.left:
            stack.append(node.left)
        output.append(node.value)

    print(f"\nThe iterative dfs postorder solution is {output}")    
    pass


def breadthFirstTraversal(root):
    """
    BFS Traversal (level-order)
    Args:
        root: TreeNode - the root of the tree
    Returns:
        list - values in BFS order
    """

    """
    Creates a sample tree:
            1
           / \
          2    3
         / \  / \
        4   5 6  7
    """

    out = []

    if root == None:
        return []

    queue = [root]

    while len(queue) > 0:
        
        node = queue.pop(0)

        out.append(node.value)

        if node.left:
            queue.append(node.left)

        if node.right:
            queue.append(node.right)

    print(f"\nThe bfs solution is {out}")


    pass


# Helper function to create a sample tree for testing
def create_sample_tree():
    """
    Creates a sample tree:
            1
           / \
          2    3
         / \  / \
        4   5 6  7
    """
    root = TreeNode(1)
    root.left = TreeNode(2)
    root.right = TreeNode(3)
    root.left.left = TreeNode(4)
    root.left.right = TreeNode(5)
    root.right.left = TreeNode(6)
    root.right.right = TreeNode(7)
    return root


if __name__ == "__main__":
    # Test your implementations here
    root = create_sample_tree()
    print("\nPre Order is when you visit Root first, then go left, and then go right")
    depthFirstTraversal(root)

    print("\nPost order is when you visit all the way right first, go back to root, then go left")
    dfs_inorder(root)

    print("\nPost order is when you vist all the way right, then left, then the root")
    dfs_postorder(root)

    dfs_iterative_preorder(root)

    dfs_iterative_inorder(root)

    dfs_iterative_postorder(root)

    breadthFirstTraversal(root)

    
    # print("=" * 50)
    # print("DFS TRAVERSAL DEMONSTRATIONS")
    # print("=" * 50)
    # print(f"Tree structure:")
    # print("        1")
    # print("       / \\")
    # print("      2   3")
    # print("     / \\ /")
    # print("    4  5 6")
    # print()
    
    # print("DFS Preorder (Root -> Left -> Right):")
    # print(f"  Recursive: {depthFirstTraversal(root)}")
    # print(f"  Iterative: {dfs_iterative_preorder(root)}")
    # print(f"  Expected:  [1, 2, 4, 5, 3, 6]")
    # print()
    
    # print("DFS Inorder (Left -> Root -> Right):")
    # print(f"  Result:    {dfs_inorder(root)}")
    # print(f"  Expected:  [4, 2, 5, 1, 6, 3]")
    # print()
    
    # print("DFS Postorder (Left -> Right -> Root):")
    # print(f"  Result:    {dfs_postorder(root)}")
    # print(f"  Expected:  [4, 5, 2, 6, 3, 1]")
    # print()
    
    # print("BFS Traversal:", breadthFirstTraversal(root))