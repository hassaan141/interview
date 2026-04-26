#include<iostream>
using namespace std;

struct Node{
  int data;
  Node* next;
};

void insertEnd (Node *&head, int value){
  Node *newEnd = new Node;
  newEnd->data = value;
  newEnd->next = nullptr;


  Node *traverse = head;

  while(traverse->next != nullptr){
    traverse = traverse->next;
  }

  traverse->next = newEnd;
}

void insertHead(Node *&head, int value){
  Node *newStart = new Node;
  newStart->data = value;
  newStart->next = head;
  head = newStart;
}

void printList(Node *head){
  Node *traverse = head;
  while(traverse != nullptr){
    cout<<traverse->data<<" ";
    traverse = traverse->next;
  }
}

int main(){
  Node *head = new Node;
  head->data = 1;
  head->next = nullptr;

  insertEnd(head, 2);
  printList(head);
  insertEnd(head, 3);
  printList(head);
  insertEnd(head, 4);
  printList(head);
  insertEnd(head, 5);
  printList(head);

}