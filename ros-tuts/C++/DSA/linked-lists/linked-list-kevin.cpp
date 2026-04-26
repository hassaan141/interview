#include<iostream>

//creating the foundation for the linked list using the Node class
struct Node{
  int data;
  Node *next;
};

typedef Node* nodePtr;

//creating the linked list class
class LinkedList{
  public:
    LinkedList(){
      head = nullptr;
    }

    void addFront(int value){
      nodePtr traverse = new Node;
      traverse->data = value;
      traverse->next = head;
      head = traverse;
    }

    void printList(){
      nodePtr traverse = head;
      while(traverse != nullptr){
        std::cout<<traverse->data<<" ";
        traverse = traverse->next;
      }
    }

    void removeNode(int value){
      nodePtr traverse = head;

      while()
    }

  private:
    Node *head;
};

int main (){
  LinkedList list;

  list.addFront(1);
  list.addFront(2);
  list.addFront(3);
  list.addFront(4);
  list.addFront(5);
  list.addFront(6);
  list.printList();
  return 0;
 
}
