"""
Tries are a tree data structure that is used to store a dynamic set of strings.
Tries are also known as prefix trees or radix trees.
Tries are used to store a dynamic set of strings.
Tries are used to store a dynamic set of strings.
"""

class TrieNode:
    def __init__(self):
        self.is_end_of_word = False
        self.children = {}

class Trie:

    def __init__(self):
        self.root = TrieNode()

    def insert(self, word: str) -> None:

        current_node = self.root 

        for char in word:
            if char not in current_node.children:
                current_node.children[char] = TrieNode()
            
            current_node = current_node.children[char]

        current_node.is_end_of_word = True
            
    def search(self, word: str) -> bool:

        curr_node = self.root 

        for char in word:

            if char not in curr_node.children:
                return False
            
            curr_node = curr_node.children[char]
        
        return children.is_end_of_word


    def delete(self, word: str) -> None:

    def has_prefix(self, prefix: str) -> bool:
        
        curr_node = self.root 

        for char in word:

            if char not in curr_node.children:
                return False
            
            curr_node = curr_node.children[char]
        
        return True

    def starts_with(self, prefix: str) -> bool:

    def list_words(self) -> list[str]:


# hello